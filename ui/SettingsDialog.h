#ifndef DIZITABFORGE_UI_SETTINGSDIALOG_H
#define DIZITABFORGE_UI_SETTINGSDIALOG_H

#include <QDialog>
#include <QString>

#include "core/config/ConfigRepository.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SettingsDialog;
}
QT_END_NAMESPACE

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(const dizi::AppConfig &config, QWidget *parent = nullptr);

    ~SettingsDialog() override;

private slots:
    void saveSettings();

private:
    dizi::AppConfig collect() const;

    void showStatus(const QString &message);

    Ui::SettingsDialog *ui = nullptr;
    QString m_filePath;
};

#endif
