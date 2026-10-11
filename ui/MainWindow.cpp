#include "ui/MainWindow.h"

#include "ui_MainWindow.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QShortcut>
#include <QTextStream>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <functional>

#include "core/network/GmlClient.h"
#include "core/network/ImageEncoder.h"
#include "core/parse/ScorePipeline.h"
#include "core/util/ImageUtil.h"
#include "core/util/PathUtil.h"
#include "core/util/ProgressTicker.h"
#include "ui/CropDialog.h"
#include "ui/SettingsDialog.h"

namespace {

constexpr int kThumbnailBound = 300;
constexpr int kMaxErrorLines = 6;

QString promptPreamble() {
    return QStringLiteral(
            "你是笛子简谱（数字谱）识别助手。任务：只把图片里的简谱转写成纯文本数字谱。"
            "除简谱本身以外的内容一律完全忽略、一个字都不要输出，包括但不限于："
            "歌词与歌词文字、曲名标题、作者/编曲/演唱/制谱者、调号与拍号的说明文字、页码、"
            "水印、二维码、印章、logo、指法与演奏提示文字、五线谱及五线谱上的音符与谱号、"
            "和弦记号、速度/力度/表情记号（如 ♩=90、D.C.、ff）、乐器与段落说明、"
            "背景图案、页边批注与手写笔记。"
            "简谱行里的非音符符号也要忽略：小节线、反复记号、跳房子记号、连音线、"
            "换气记号、装饰音标记。"
            "两种小圆点要分清：写在数字正下方的点表示低八度，转写成数字后的逗号，例如 6,；"
            "写在数字右边的点是附点（时值加半拍），本程序不表示，直接省略不写。"
            "记法规则：中音区直接写数字 1-7；高音区在数字后加单引号，例如 1'；"
            "低音区在数字后加逗号，例如 6,；减号 - 表示延音线，延长前面一个音一拍；"
            "0 表示休止符；# 表示升号、b 表示降号，写在数字前面。");
}

QString promptRangeGuard() {
    return QStringLiteral(
            "音域校验（很重要）：本谱是筒音作 5，笛子实际能吹的音只有下面这些"
            "（这一串由程序内置指法表生成，不要输出此外的任何音）：");
}

QString promptInferenceRules() {
    return QStringLiteral(
            "如果你读出的音落在这个范围之外（例如更低音区的数字，或者比 6' 更高的音），"
            "那一定是识别误差，不要原样输出这个吹不出来的音，而要推测它原本是什么音，"
            "并且只输出推测后的结果："
            "第一优先，怀疑八度记号被误读——去掉多余的低音逗号或高音单引号，取同数字的中音，"
            "例如 3, 按 3 输出、7' 按 7 输出；"
            "第二，如果去掉八度记号后与前后音明显冲突（旋律跳进突兀、调式不合），"
            "就按音阶级进选最接近的那个合法音，例如低音 4, 更可能是 5,；"
            "第三，无论如何输出都必须落在上面列出的音之内，不要输出 ? 或任何说明文字。"
            "输出格式：每行简谱输出为一行，音符之间用一个空格分隔；"
            "只输出这些记法本身，不要解释、不要表格、不要标点、不要 Markdown 代码块。");
}

QString buildPrompt() {
    const QString range = QString::fromStdString(dizi::playableRangeText());
    return promptPreamble() + promptRangeGuard() + range + QStringLiteral("。") +
           promptInferenceRules();
}

struct WorkerResult {
    QStringList notations;
    QString error;
};

QStringList missingConfigFields(const dizi::AppConfig &config) {
    QStringList missing;
    if (config.apiKey.trimmed().isEmpty()) {
        missing.append(QStringLiteral("API Key"));
    }
    if (config.endpoint.trimmed().isEmpty()) {
        missing.append(QStringLiteral("接口地址"));
    }
    if (config.model.trimmed().isEmpty()) {
        missing.append(QStringLiteral("模型名称"));
    }
    return missing;
}

bool prepareImages(const QStringList &paths, dizi::GmlRequest &request, QString &error) {
    const dizi::ImageEncoder encoder;
    for (const QString &path : paths) {
        dizi::EncodedImage encoded;
        if (!encoder.encode(path, encoded, error)) {
            return false;
        }
        request.imageDataUris.append(encoded.dataUri);
    }
    return true;
}

bool runRecognition(const dizi::AppConfig &config, const QStringList &paths, WorkerResult &result,
                    const dizi::GmlClient::ProgressCallback &onProgress) {
    dizi::GmlRequest request;
    request.endpoint = config.endpoint;
    request.apiKey = config.apiKey;
    request.model = config.model;
    request.timeoutSeconds = config.timeoutSeconds;
    request.prompt = buildPrompt();
    QString error;
    if (!prepareImages(paths, request, error)) {
        result.error = error;
        return false;
    }
    dizi::GmlClient client;
    client.setRequest(request);
    client.setProgressCallback(onProgress);
    const dizi::GmlResponse response = client.send();
    if (!response.ok) {
        result.error = response.error;
        return false;
    }
    result.notations.append(response.content);
    return true;
}

QAction *addActionWithIcon(QMenu &menu, const QString &text, const QString &iconPath) {
    QAction *action = menu.addAction(text);
    action->setIcon(QIcon(iconPath));
    return action;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow()) {
    ui->setupUi(this);
    connectSignals();
    loadConfig();
    refreshPages();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::connectSignals() {
    connect(ui->generateButton, &QPushButton::clicked, this, &MainWindow::generatePdf);
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(ui->importButton, &QPushButton::clicked, this, &MainWindow::chooseImages);
    connect(ui->addMoreButton, &QPushButton::clicked, this, &MainWindow::chooseImages);
    connect(ui->thumbList, &QListWidget::itemDoubleClicked, this, &MainWindow::openThumbnail);
    connect(ui->thumbList, &QListWidget::customContextMenuRequested, this,
            &MainWindow::showThumbnailMenu);
    connect(&m_watcher, &QFutureWatcher<int>::finished, this, &MainWindow::finishGeneration);
    connect(&m_ticker, &dizi::ProgressTicker::percentChanged, this, &MainWindow::reportPercent);
    auto *shortcut = new QShortcut(QKeySequence::Delete, ui->thumbList);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::removeSelectedItem);
}

void MainWindow::loadConfig() {
    dizi::ConfigRepository repository(path_util::configFilePath());
    QString error;
    repository.load(m_config, error);
    const QStringList missing = missingConfigFields(m_config);
    if (!missing.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("尚未设置 %1，请点击右上角「设置」")
                                         .arg(missing.join(QStringLiteral("、"))));
    }
}

void MainWindow::refreshPages() {
    const bool empty = m_imagePaths.isEmpty();
    ui->mainStack->setCurrentWidget(empty ? ui->emptyPage : ui->listPage);
    ui->generateButton->setEnabled(!empty);
    if (!empty) {
        ui->statusLabel->setText(
                QStringLiteral("已导入 %1 张图片，可以开始识别").arg(m_imagePaths.size()));
    }
}

QListWidgetItem *MainWindow::makeThumbnailItem(const QString &path) const {
    auto *item = new QListWidgetItem(QFileInfo(path).fileName());
    item->setData(Qt::UserRole, path);
    item->setToolTip(QStringLiteral("%1\n双击裁剪，右键删除").arg(path_util::describe(path)));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    return item;
}

void MainWindow::updateThumbnail(int index) {
    QListWidgetItem *item = ui->thumbList->item(index);
    if (item == nullptr) {
        return;
    }
    QImage image;
    QString error;
    image_util::loadImage(item->data(Qt::UserRole).toString(), image, error);
    const QSize fitted = image_util::scaledSizeToBound(image.width(), image.height(),
                                                      kThumbnailBound);
    item->setIcon(QIcon(QPixmap::fromImage(
            image.scaled(fitted, Qt::KeepAspectRatio, Qt::SmoothTransformation))));
}

void MainWindow::addThumbnail(const QString &path) {
    ui->thumbList->addItem(makeThumbnailItem(path));
    updateThumbnail(ui->thumbList->count() - 1);
}

void MainWindow::addImages(const QStringList &paths) {
    for (const QString &path : paths) {
        if (m_imagePaths.contains(path)) {
            continue;
        }
        m_imagePaths.append(path);
        addThumbnail(path);
    }
    refreshPages();
}

void MainWindow::chooseImages() {
    const QStringList paths = QFileDialog::getOpenFileNames(
            this, QStringLiteral("选择简谱图片"), QDir::homePath(),
            QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"));
    if (!paths.isEmpty()) {
        addImages(paths);
    }
}

void MainWindow::replaceSelectedPath(const QString &path) {
    const int index = ui->thumbList->currentRow();
    if (index < 0 || index >= m_imagePaths.size()) {
        return;
    }
    const QString previous = m_imagePaths.at(index);
    m_imagePaths.replace(index, path);
    delete ui->thumbList->takeItem(index);
    ui->thumbList->insertItem(index, makeThumbnailItem(path));
    updateThumbnail(index);
    ui->thumbList->setCurrentRow(index);
    ui->statusLabel->setText(QStringLiteral("已用裁剪结果替换 %1")
                                     .arg(QFileInfo(previous).fileName()));
}

void MainWindow::openThumbnail(QListWidgetItem *item) {
    if (item == nullptr) {
        return;
    }
    CropDialog dialog(item->data(Qt::UserRole).toString(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    replaceSelectedPath(dialog.resultPath());
}

void MainWindow::removeSelectedItem() {
    const int index = ui->thumbList->currentRow();
    if (index < 0) {
        return;
    }
    delete ui->thumbList->takeItem(index);
    m_imagePaths.removeAt(index);
    refreshPages();
}

void MainWindow::showThumbnailMenu(const QPoint &position) {
    QListWidgetItem *item = ui->thumbList->itemAt(position);
    if (item == nullptr) {
        return;
    }
    ui->thumbList->setCurrentItem(item);
    QMenu menu(this);
    QAction *cropAction = addActionWithIcon(menu, QStringLiteral("裁剪"),
                                            QStringLiteral(":/edit.png"));
    QAction *removeAction = menu.addAction(QStringLiteral("删除"));
    const QAction *chosen = menu.exec(ui->thumbList->viewport()->mapToGlobal(position));
    if (chosen == cropAction) {
        openThumbnail(item);
    }
    if (chosen == removeAction) {
        removeSelectedItem();
    }
}

void MainWindow::openSettings() {
    SettingsDialog dialog(m_config, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    loadConfig();
    ui->statusLabel->setText(QStringLiteral("设置已保存到 %1")
                                     .arg(path_util::describe(path_util::configFilePath())));
}

void MainWindow::generatePdf() {
    if (m_imagePaths.isEmpty()) {
        showError(QStringLiteral("请先导入至少一张简谱图片。"));
        return;
    }
    loadConfig();
    const QStringList missing = missingConfigFields(m_config);
    if (!missing.isEmpty()) {
        showError(QStringLiteral("还没有设置 %1，请先在「设置」里填写。")
                          .arg(missing.join(QStringLiteral("、"))));
        return;
    }
    startRecognition();
}

void MainWindow::startRecognition() {
    ui->generateButton->setEnabled(false);
    ui->progressBar->setRange(0, 100);
    ui->statusLabel->setText(QStringLiteral("正在识别 %1 张图片…").arg(m_imagePaths.size()));
    m_ticker.begin();
    const QStringList paths = m_imagePaths;
    const dizi::AppConfig config = m_config;
    m_outcome = RecognitionOutcome();
    auto *result = &m_outcome;
    auto callback = [this](qint64 received, qint64 total) {
        QMetaObject::invokeMethod(this, std::function<void()>([this, received, total]() {
            reportProgress(received, total);
        }), Qt::QueuedConnection);
    };
    m_watcher.setFuture(QtConcurrent::run([result, config, paths, callback]() {
        WorkerResult partial;
        runRecognition(config, paths, partial, callback);
        result->notations = partial.notations;
        if (!partial.error.isEmpty()) {
            result->error = partial.error;
        }
        return 0;
    }));
}

void MainWindow::reportProgress(qint64 received, qint64 total) {
    m_ticker.reportUpload(received, total);
}

void MainWindow::reportPercent(int percent) {
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(percent);
    ui->statusLabel->setText(QStringLiteral("正在识别 %1 张图片… %2%")
                                     .arg(m_imagePaths.size())
                                     .arg(percent));
}

void MainWindow::finishGeneration() {
    m_ticker.complete();
    ui->generateButton->setEnabled(!m_imagePaths.isEmpty());
    if (m_outcome.error.has_value()) {
        showError(*m_outcome.error);
        return;
    }
    importNotations(m_outcome.notations);
}

QString MainWindow::choosePdfOutputPath() const {
    const QString suggested = m_imagePaths.isEmpty()
                                      ? path_util::defaultOutputPdfPath(QString())
                                      : path_util::defaultOutputPdfPath(m_imagePaths.first());
    const QString chosen = QFileDialog::getSaveFileName(
            const_cast<MainWindow *>(this), QStringLiteral("保存 PDF"), suggested,
            QStringLiteral("PDF 文件 (*.pdf)"));
    if (chosen.isEmpty()) {
        return QString();
    }
    if (path_util::isPdfPath(chosen)) {
        return chosen;
    }
    return chosen + QStringLiteral(".pdf");
}

void MainWindow::reportWarnings(const QStringList &failures) {
    if (failures.isEmpty()) {
        return;
    }
    const QString details = failures.mid(0, kMaxErrorLines).join(QStringLiteral("\n"));
    ui->statusLabel->setText(
            QStringLiteral("已生成 PDF，但有 %1 处识别结果无法解析").arg(failures.size()));
    QMessageBox::information(this, QStringLiteral("部分识别结果被跳过"),
                             QStringLiteral("%1\n\n已跳过这些片段，其余简谱仍写入 PDF。")
                                     .arg(details));
}

std::vector<dizi::Score> MainWindow::buildScores(const QStringList &notations,
                                                 QStringList &failures) const {
    std::vector<dizi::Score> scores;
    for (int index = 0; index < notations.size(); ++index) {
        dizi::Score parsed;
        dizi::ScorePipeline pipeline;
        if (!pipeline.parse(notations.at(index).toStdString(), parsed)) {
            failures.append(QStringLiteral("第 %1 张：%2")
                                    .arg(index + 1)
                                    .arg(QString::fromStdString(pipeline.error())));
            continue;
        }
        scores.push_back(parsed);
    }
    return scores;
}

void MainWindow::importNotations(const QStringList &notations) {
    QStringList failures;
    const std::vector<dizi::Score> scores = buildScores(notations, failures);
    if (scores.empty()) {
        showError(QStringLiteral("识别结果无法解析为简谱记法。\n%1")
                          .arg(failures.mid(0, kMaxErrorLines).join(QStringLiteral("\n"))));
        return;
    }
    const QString outputPath = choosePdfOutputPath();
    if (outputPath.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("已取消保存"));
        return;
    }
    QString error;
    if (!m_renderer.render(scores, outputPath, error)) {
        showError(error);
        return;
    }
    ui->statusLabel->setText(QStringLiteral("已生成 %1（%2 张简谱）")
                                     .arg(path_util::describe(outputPath))
                                     .arg(scores.size()));
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
    reportWarnings(failures);
}

void MainWindow::showError(const QString &message) {
    ui->statusLabel->setText(QStringLiteral("生成失败"));
    QMessageBox::warning(this, QStringLiteral("生成 PDF 失败"), message);
}
