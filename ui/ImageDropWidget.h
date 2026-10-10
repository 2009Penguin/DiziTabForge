#ifndef DIZITABFORGE_UI_IMAGEDROPWIDGET_H
#define DIZITABFORGE_UI_IMAGEDROPWIDGET_H

#include <QStringList>
#include <QWidget>

class ImageDropWidget : public QWidget {
    Q_OBJECT

public:
    explicit ImageDropWidget(QWidget *parent = nullptr);

signals:
    void imagesDropped(const QStringList &paths);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

private:
    void setHighlighted(bool highlighted);

    bool m_highlighted = false;
};

#endif
