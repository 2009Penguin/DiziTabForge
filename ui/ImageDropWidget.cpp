#include "ui/ImageDropWidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QStyle>
#include <QUrl>

#include "core/util/PathUtil.h"

ImageDropWidget::ImageDropWidget(QWidget *parent)
    : QWidget(parent) {
    setProperty("dropActive", false);
}

void ImageDropWidget::setHighlighted(bool highlighted) {
    if (m_highlighted == highlighted) {
        return;
    }
    m_highlighted = highlighted;
    setProperty("dropActive", highlighted);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ImageDropWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (!event->mimeData()->hasUrls()) {
        return;
    }
    event->acceptProposedAction();
    setHighlighted(true);
}

void ImageDropWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    event->accept();
    setHighlighted(false);
}

void ImageDropWidget::dropEvent(QDropEvent *event) {
    setHighlighted(false);
    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        const QString local = url.toLocalFile();
        if (!local.isEmpty() && path_util::hasImageSuffix(local)) {
            paths.append(QFileInfo(local).absoluteFilePath());
        }
    }
    if (paths.isEmpty()) {
        return;
    }
    event->acceptProposedAction();
    emit imagesDropped(paths);
}
