#include "ui/MainWindow.h"

#include "ui_MainWindow.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QShortcut>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <functional>

#include "core/network/GmlClient.h"
#include "core/network/ImageEncoder.h"
#include "core/util/ImageUtil.h"
#include "core/util/PathUtil.h"
#include "ui/CropDialog.h"
#include "ui/ImageDropWidget.h"
#include "ui/SettingsDialog.h"

namespace {

constexpr int kThumbnailBound = 300;
constexpr int kMaxErrorLines = 6;

const QString kPrompt = QStringLiteral(
        "你是简谱识别助手。请把图片中的笛子简谱逐行转写成纯文本数字谱，"
        "只输出记法本身，不要解释、不要表格、不要标点。"
        "规则：中音区直接写数字 1-7；高音区在数字后加单引号，例如 1'；"
        "低音区在数字后加逗号，例如 6,；减号 - 表示延音线，延长前面一个音一拍；"
        "0 表示休止符；# 表示升号、b 表示降号，写在数字前面。"
        "每行简谱输出为一行，音符之间用一个空格分隔。");

struct WorkerResult {
    QStringList notations;
    QString error;
};

bool prepareImages(const QStringList &paths, GmlRequest &request, QString &error) {
    const ImageEncoder encoder;
    for (const QString &path : paths) {
        EncodedImage encoded;
        if (!encoder.encode(path, encoded, error)) {
            return false;
        }
        request.imageDataUris.append(encoded.dataUri);
    }
    return true;
}

bool runRecognition(const dizi::AppConfig &config, const QStringList &paths, WorkerResult &result,
                    const dizi::GmlClient::ProgressCallback &onProgress) {
    GmlRequest request;
    request.endpoint = config.endpoint;
    request.apiKey = config.apiKey;
    request.model = config.model;
    request.timeoutSeconds = config.timeoutSeconds;
    request.prompt = kPrompt;
    QString error;
    if (!prepareImages(paths, request, error)) {
        result.error = error;
        return false;
    }
    dizi::GmlClient client;
    client.setRequest(request);
    client.setProgressCallback(onProgress);
    const GmlResponse response = client.send();
    if (!response.ok) {
        result.error = response.error;
        return false;
    }
    result.notations.append(response.content);
    return true;
}

bool renderPdf(const QString &outputPath, const dizi::Score &score, dizi::PdfRenderer &renderer,
               QString &error) {
    dizi::ScorePage page;
    page.caption = QStringLiteral("识别结果");
    page.notes = score;
    return renderer.render({page}, outputPath, error);
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
    connect(ui->dropArea, &ImageDropWidget::imagesDropped, this, &MainWindow::addImages);
    connect(ui->thumbList, &QListWidget::itemDoubleClicked, this, &MainWindow::openThumbnail);
    connect(ui->thumbList, &QListWidget::customContextMenuRequested, this,
            &MainWindow::showThumbnailMenu);
    connect(&m_watcher, &QFutureWatcher<int>::finished, this, &MainWindow::finishGeneration);
    ui->thumbList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->thumbList->setViewMode(QListView::IconMode);
    auto *shortcut = new QShortcut(QKeySequence::Delete, ui->thumbList);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::removeSelectedItem);
}

void MainWindow::loadConfig() {
    dizi::ConfigRepository repository(path_util::configFilePath());
    QString error;
    repository.load(m_config, error);
    if (m_config.apiKey.isEmpty()) {
        ui->statusLabel->setText(QStringLiteral("尚未设置 API Key，请点击右上角「设置」"));
    }
}

void MainWindow::refreshPages() {
    const bool empty = m_imagePaths.isEmpty();
    ui->mainStack->setCurrentWidget(empty ? ui->dropPage : ui->listPage);
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
    QAction *cropAction = menu.addAction(QStringLiteral("裁剪"));
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
        showError(QStringLiteral("请先拖入至少一张简谱图片。"));
        return;
    }
    loadConfig();
    if (m_config.apiKey.isEmpty()) {
        showError(QStringLiteral("还没有设置 API Key，请先在「设置」里填写。"));
        return;
    }
    startRecognition();
}

void MainWindow::startRecognition() {
    ui->generateButton->setEnabled(false);
    ui->mainStack->setCurrentWidget(ui->busyPage);
    ui->progressBar->setRange(0, 0);
    ui->statusLabel->setText(QStringLiteral("正在识别 %1 张图片…").arg(m_imagePaths.size()));
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
    if (total <= 0) {
        return;
    }
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(static_cast<int>(received * 100 / total));
    ui->statusLabel->setText(QStringLiteral("已上传 %1 KB / %2 KB")
                                     .arg(received / 1024)
                                     .arg(total / 1024));
}

void MainWindow::finishGeneration() {
    ui->generateButton->setEnabled(!m_imagePaths.isEmpty());
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->mainStack->setCurrentWidget(m_imagePaths.isEmpty() ? ui->dropPage : ui->listPage);
    if (m_outcome.error.has_value()) {
        showError(*m_outcome.error);
        return;
    }
    importNotations(m_outcome.notations);
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

void MainWindow::importNotations(const QStringList &notations) {
    m_pendingScore.clear();
    QStringList failures;
    for (int index = 0; index < notations.size(); ++index) {
        dizi::Score parsed;
        if (!m_pipeline.parse(notations.at(index).toStdString(), parsed)) {
            failures.append(QStringLiteral("第 %1 张：%2")
                                    .arg(index + 1)
                                    .arg(QString::fromStdString(m_pipeline.error())));
            continue;
        }
        m_pendingScore.insert(m_pendingScore.end(), parsed.begin(), parsed.end());
    }
    if (m_pendingScore.empty()) {
        const QString details = failures.mid(0, kMaxErrorLines).join(QStringLiteral("\n"));
        showError(QStringLiteral("识别结果无法解析为简谱记法。\n%1").arg(details));
        return;
    }
    const QString outputPath = path_util::defaultOutputPdfPath(m_imagePaths.first());
    QString error;
    if (!renderPdf(outputPath, m_pendingScore, m_renderer, error)) {
        showError(error);
        return;
    }
    ui->statusLabel->setText(QStringLiteral("已生成 %1").arg(path_util::describe(outputPath)));
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
    reportWarnings(failures);
}

void MainWindow::showError(const QString &message) {
    ui->statusLabel->setText(QStringLiteral("生成失败"));
    QMessageBox::warning(this, QStringLiteral("生成 PDF 失败"), message);
}
