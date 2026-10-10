#include "ui/SettingsDialog.h"

#include "ui_SettingsDialog.h"

#include <QDialog>
#include <QPushButton>

#include "core/util/PathUtil.h"

SettingsDialog::SettingsDialog(const dizi::AppConfig &config, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::SettingsDialog()) {
    ui->setupUi(this);
    connect(ui->saveButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    ui->apiKeyEdit->setText(config.apiKey);
    ui->endpointEdit->setText(config.endpoint);
    ui->modelEdit->setText(config.model);
    ui->timeoutSpin->setValue(config.timeoutSeconds);
    m_filePath = path_util::configFilePath();
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}

dizi::AppConfig SettingsDialog::collect() const {
    dizi::AppConfig config = dizi::ConfigRepository::defaults();
    config.apiKey = ui->apiKeyEdit->text().trimmed();
    config.endpoint = ui->endpointEdit->text().trimmed();
    config.model = ui->modelEdit->text().trimmed();
    config.timeoutSeconds = ui->timeoutSpin->value();
    return config;
}

void SettingsDialog::showStatus(const QString &message) {
    ui->statusLabel->setText(message);
}

void SettingsDialog::saveSettings() {
    const dizi::AppConfig config = collect();
    if (config.apiKey.isEmpty()) {
        showStatus(QStringLiteral("API Key cannot be empty."));
        return;
    }
    if (config.endpoint.isEmpty()) {
        showStatus(QStringLiteral("Endpoint cannot be empty."));
        return;
    }
    dizi::ConfigRepository repository(m_filePath);
    QString error;
    if (!repository.save(config, error)) {
        showStatus(error);
        return;
    }
    accept();
}
