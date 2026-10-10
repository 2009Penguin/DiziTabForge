#ifndef DIZITABFORGE_UI_CROPDIALOG_H
#define DIZITABFORGE_UI_CROPDIALOG_H

#include <QDialog>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class CropDialog;
}
class QEvent;
class QShowEvent;
QT_END_NAMESPACE

class CropDialog : public QDialog {
    Q_OBJECT

public:
    explicit CropDialog(const QString &imagePath, QWidget *parent = nullptr);

    ~CropDialog() override;

    QString resultPath() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    void showEvent(QShowEvent *event) override;

private slots:
    void applyCrop();

    void resetSelection();

private:
    void refreshPreview();

    void setAnchor(const QPoint &position);

    void setCurrent(const QPoint &position);

    QPoint toPreviewPoint(const QPoint &labelPoint) const;

    QRect selectionInWidget() const;

    QRect selectionInImage() const;

    void showError(const QString &message);

    Ui::CropDialog *ui = nullptr;
    QImage m_source;
    QImage m_preview;
    QPoint m_anchor;
    QPoint m_current;
    bool m_dragging = false;
    QString m_sourcePath;
    QString m_resultPath;
};

#endif
