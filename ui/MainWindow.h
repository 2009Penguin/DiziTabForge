#ifndef DIZITABFORGE_UI_MAINWINDOW_H
#define DIZITABFORGE_UI_MAINWINDOW_H

#include <QFutureWatcher>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QStringList>

#include <optional>

#include "core/config/ConfigRepository.h"
#include "core/model/Note.h"
#include "core/parse/ScorePipeline.h"
#include "core/render/PdfRenderer.h"

class QPoint;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct RecognitionOutcome {
    QStringList notations;
    std::optional<QString> error;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

private slots:
    void addImages(const QStringList &paths);

    void chooseImages();

    void openSettings();

    void generatePdf();

    void removeSelectedItem();

    void showThumbnailMenu(const QPoint &position);

    void openThumbnail(QListWidgetItem *item);

    void reportProgress(qint64 received, qint64 total);

    void finishGeneration();

private:
    void connectSignals();

    void loadConfig();

    void refreshPages();

    void startRecognition();

    void showError(const QString &message);

    void importNotations(const QStringList &notations);

    void reportWarnings(const QStringList &failures);

    void addThumbnail(const QString &path);

    void updateThumbnail(int index);

    QListWidgetItem *makeThumbnailItem(const QString &path) const;

    void replaceSelectedPath(const QString &path);

    Ui::MainWindow *ui = nullptr;
    dizi::AppConfig m_config;
    QStringList m_imagePaths;
    dizi::Score m_pendingScore;
    dizi::ScorePipeline m_pipeline;
    dizi::PdfRenderer m_renderer;
    QFutureWatcher<int> m_watcher;
    RecognitionOutcome m_outcome;
};

#endif
