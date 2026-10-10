#ifndef DIZITABFORGE_CORE_UTIL_PATHUTIL_H
#define DIZITABFORGE_CORE_UTIL_PATHUTIL_H

#include <QString>

namespace path_util {

QString applicationDir();

QString configFilePath();

QString describe(const QString &path);

QString resolveOutputPdfPath(const QString &requested, const QString &firstImage);

QString defaultOutputPdfPath(const QString &firstImage);

QString siblingFilePath(const QString &sourcePath, const QString &prefix);

bool isPdfPath(const QString &path);

bool hasImageSuffix(const QString &path);

QString normalizeChatEndpoint(const QString &endpoint);

}

#endif
