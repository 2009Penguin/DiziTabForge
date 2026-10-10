#ifndef DIZITABFORGE_CORE_CONFIG_CONFIGREPOSITORY_H
#define DIZITABFORGE_CORE_CONFIG_CONFIGREPOSITORY_H

#include <QString>

namespace dizi {

struct AppConfig {
    QString apiKey;
    QString endpoint;
    QString model;
    int timeoutSeconds = 90;
};

class ConfigRepository {
public:
    explicit ConfigRepository(QString filePath);

    QString filePath() const;

    bool load(AppConfig &target, QString &error) const;

    bool save(const AppConfig &config, QString &error) const;

    static AppConfig defaults();

private:
    QString m_filePath;
};

}

#endif
