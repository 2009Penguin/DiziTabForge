#include "core/util/ImageUtil.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>

namespace image_util {

bool loadImage(const QString &path, QImage &target, QString &error) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage loaded;
    if (!reader.read(&loaded) || loaded.isNull()) {
        error = QStringLiteral("Cannot read image: %1 (%2)")
                        .arg(path, reader.errorString());
        return false;
    }
    target = loaded;
    error.clear();
    return true;
}

bool savePng(const QImage &image, const QString &path, QString &error) {
    if (image.isNull()) {
        error = QStringLiteral("Refusing to write an empty image to %1").arg(path);
        return false;
    }
    QImageWriter writer(path, "png");
    if (!writer.write(image)) {
        error = QStringLiteral("Cannot write %1 (%2)").arg(path, writer.errorString());
        return false;
    }
    error.clear();
    return true;
}

QByteArray encodeBase64(const QByteArray &raw) {
    return raw.toBase64();
}

QByteArray toPngBytes(const QImage &image) {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    return bytes;
}

QString mimeTypeOf(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")) {
        return QStringLiteral("image/jpeg");
    }
    if (suffix == QStringLiteral("webp")) {
        return QStringLiteral("image/webp");
    }
    if (suffix == QStringLiteral("bmp")) {
        return QStringLiteral("image/bmp");
    }
    if (suffix == QStringLiteral("gif")) {
        return QStringLiteral("image/gif");
    }
    return QStringLiteral("image/png");
}

QSize fitSize(const QSize &source, const QSize &bounds) {
    if (source.isEmpty() || bounds.isEmpty()) {
        return QSize();
    }
    return source.scaled(bounds, Qt::KeepAspectRatio);
}

QRect clampRectTo(const QRect &rect, const QSize &bounds) {
    const QRect outer(QPoint(0, 0), bounds);
    return rect.normalized().intersected(outer);
}

QRect normalizedRect(const QPoint &anchor, const QPoint &current) {
    return QRect(anchor, current).normalized();
}

QRect mapToImage(const QRect &widgetRect, const QSize &widgetSize, const QSize &imageSize) {
    const QSize fitted = fitSize(imageSize, widgetSize);
    if (fitted.isEmpty()) {
        return QRect();
    }
    const double scaleX = static_cast<double>(imageSize.width()) / fitted.width();
    const double scaleY = static_cast<double>(imageSize.height()) / fitted.height();
    const QPoint origin((widgetSize.width() - fitted.width()) / 2,
                        (widgetSize.height() - fitted.height()) / 2);
    const QRect local = widgetRect.translated(-origin);
    const QRect scaled(static_cast<int>(local.x() * scaleX),
                       static_cast<int>(local.y() * scaleY),
                       static_cast<int>(local.width() * scaleX),
                       static_cast<int>(local.height() * scaleY));
    return clampRectTo(scaled, imageSize);
}

QRect mapToWidget(const QRect &imageRect, const QSize &widgetSize, const QSize &imageSize) {
    const QSize fitted = fitSize(imageSize, widgetSize);
    if (fitted.isEmpty()) {
        return QRect();
    }
    const double scaleX = static_cast<double>(fitted.width()) / imageSize.width();
    const double scaleY = static_cast<double>(fitted.height()) / imageSize.height();
    const QPoint origin((widgetSize.width() - fitted.width()) / 2,
                        (widgetSize.height() - fitted.height()) / 2);
    return QRect(origin.x() + static_cast<int>(imageRect.x() * scaleX),
                 origin.y() + static_cast<int>(imageRect.y() * scaleY),
                 static_cast<int>(imageRect.width() * scaleX),
                 static_cast<int>(imageRect.height() * scaleY));
}

QSize scaledSizeToBound(int width, int height, int bound) {
    if (width <= 0 || height <= 0 || bound <= 0) {
        return QSize();
    }
    return QSize(width, height).scaled(bound, bound, Qt::KeepAspectRatio);
}

}
