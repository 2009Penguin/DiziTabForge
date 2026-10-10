#include "core/util/PathUtil.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "core/util/StringUtil.h"

namespace path_util {

QString applicationDir() {
    return QCoreApplication::applicationDirPath();
}

QString configFilePath() {
    return QDir(applicationDir()).filePath(QStringLiteral("config.json"));
}

QString describe(const QString &path) {
    return QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
}

QString defaultOutputPdfPath(const QString &firstImage) {
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString name = QStringLiteral("dizi-tab-%1.pdf").arg(stamp);
    if (firstImage.isEmpty()) {
        return QDir(applicationDir()).filePath(name);
    }
    return QDir(QFileInfo(firstImage).absolutePath()).filePath(name);
}

QString resolveOutputPdfPath(const QString &requested, const QString &firstImage) {
    if (requested.isEmpty()) {
        return defaultOutputPdfPath(firstImage);
    }
    if (QFileInfo(requested).isAbsolute()) {
        return requested;
    }
    return QDir(QFileInfo(firstImage).absolutePath()).filePath(requested);
}

QString siblingFilePath(const QString &sourcePath, const QString &prefix) {
    const QFileInfo info(sourcePath);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmsszzz"));
    const QString name = QStringLiteral("%1-%2.png").arg(prefix, stamp);
    return QDir(info.absolutePath()).filePath(name);
}

bool isPdfPath(const QString &path) {
    return string_util::toLower(QFileInfo(path).suffix().toStdString()) == "pdf";
}

bool hasImageSuffix(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    const QStringList known{QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
                            QStringLiteral("bmp"), QStringLiteral("webp"), QStringLiteral("gif")};
    return known.contains(suffix);
}

}
