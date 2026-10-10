#include "core/network/ImageEncoder.h"

#include <QFile>
#include <QFileInfo>

#include "core/util/ImageUtil.h"

namespace dizi {

namespace {

constexpr qint64 kMaxImageBytes = 10 * 1024 * 1024;

bool readBytes(const QString &path, QByteArray &target, QString &error) {
    QFile file(path);
    if (!file.exists()) {
        error = QStringLiteral("Image not found: %1").arg(path);
        return false;
    }
    if (file.size() > kMaxImageBytes) {
        error = QStringLiteral("%1 is larger than the 10 MB limit accepted by the model")
                        .arg(QFileInfo(path).fileName());
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Cannot read %1: %2").arg(path, file.errorString());
        return false;
    }
    target = file.readAll();
    return true;
}

}

bool ImageEncoder::encode(const QString &path, EncodedImage &target, QString &error) const {
    QByteArray raw;
    if (!readBytes(path, raw, error)) {
        return false;
    }
    if (raw.isEmpty()) {
        error = QStringLiteral("%1 is empty").arg(path);
        return false;
    }
    target.fileName = QFileInfo(path).fileName();
    target.mimeType = image_util::mimeTypeOf(path);
    target.dataUri = QStringLiteral("data:%1;base64,%2")
                             .arg(target.mimeType,
                                  QString::fromLatin1(image_util::encodeBase64(raw)));
    error.clear();
    return true;
}

}
