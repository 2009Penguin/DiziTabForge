#include "core/config/ConfigRepository.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace dizi {

ConfigRepository::ConfigRepository(QString filePath)
    : m_filePath(std::move(filePath)) {
}

QString ConfigRepository::filePath() const {
    return m_filePath;
}

bool ConfigRepository::load(AppConfig &target, QString &error) const {
    QFile file(m_filePath);
    if (!file.exists()) {
        target = AppConfig();
        error = QStringLiteral("config.json not found at %1").arg(m_filePath);
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Cannot open %1: %2").arg(m_filePath, file.errorString());
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        error = QStringLiteral("%1 is not a JSON object").arg(m_filePath);
        return false;
    }
    const QJsonObject object = document.object();
    AppConfig loaded;
    loaded.apiKey = object.value(QStringLiteral("apiKey")).toString();
    loaded.endpoint = object.value(QStringLiteral("endpoint")).toString();
    loaded.model = object.value(QStringLiteral("model")).toString();
    loaded.timeoutSeconds = object.value(QStringLiteral("timeoutSeconds")).toInt(
            loaded.timeoutSeconds);
    target = loaded;
    error.clear();
    return true;
}

bool ConfigRepository::save(const AppConfig &config, QString &error) const {
    const QDir directory = QFileInfo(m_filePath).absoluteDir();
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        error = QStringLiteral("Cannot create directory %1").arg(directory.absolutePath());
        return false;
    }
    QJsonObject object;
    object.insert(QStringLiteral("apiKey"), config.apiKey);
    object.insert(QStringLiteral("endpoint"), config.endpoint);
    object.insert(QStringLiteral("model"), config.model);
    object.insert(QStringLiteral("timeoutSeconds"), config.timeoutSeconds);
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("Cannot write %1: %2").arg(m_filePath, file.errorString());
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    file.close();
    error.clear();
    return true;
}

}
