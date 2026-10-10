#ifndef DIZITABFORGE_CORE_UTIL_IMAGEUTIL_H
#define DIZITABFORGE_CORE_UTIL_IMAGEUTIL_H

#include <QByteArray>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

namespace image_util {

bool loadImage(const QString &path, QImage &target, QString &error);

bool savePng(const QImage &image, const QString &path, QString &error);

QByteArray encodeBase64(const QByteArray &raw);

QByteArray toPngBytes(const QImage &image);

QString mimeTypeOf(const QString &path);

QSize fitSize(const QSize &source, const QSize &bounds);

QRect clampRectTo(const QRect &rect, const QSize &bounds);

QRect normalizedRect(const QPoint &anchor, const QPoint &current);

QRect mapToImage(const QRect &widgetRect, const QSize &widgetSize, const QSize &imageSize);

QRect mapToWidget(const QRect &imageRect, const QSize &widgetSize, const QSize &imageSize);

QSize scaledSizeToBound(int width, int height, int bound);

}

#endif
