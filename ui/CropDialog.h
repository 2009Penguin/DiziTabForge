#ifndef DIZITABFORGE_UI_CROPDIALOG_H
#define DIZITABFORGE_UI_CROPDIALOG_H

#include <QImage>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class CropDialog;
}
class QEvent;
QT_END_NAMESPACE

class CropDialog : public QDialog {
    Q_OBJECT

public:
    explicit CropDialog(const QString &imagePath, QWidget *parent = nullptr);

    ~CropDialog() override;

    QString resultPath() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void applyCrop();

    void resetSelection();

private:
    void refreshPreview();

    void setAnchor(const QPoint &position);

    void setCurrent(const QPoint &position);

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
