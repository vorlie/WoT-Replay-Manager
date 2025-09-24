#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H


#include <QDialog>
#include <QSettings>
#include <QDebug>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QSettings *settings, QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_executableBrowseButton_clicked();
    void on_replaysBrowseButton_clicked();
    void on_versionBrowseButton_clicked();
    void on_buttonBox_accepted();  // Add this to match UI signal
    void on_buttonBox_rejected();  // Add this to match UI signal

private:
    Ui::SettingsDialog *ui;
    QSettings *appSettings;
    void saveSettings();
};

#endif // SETTINGSDIALOG_H