#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QFileDialog>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , appSettings(settings)
{
    ui->setupUi(this);

    ui->executableLineEdit->setText(appSettings->value("executable_path").toString());
    ui->replaysLineEdit->setText(appSettings->value("replays_path").toString());
    ui->versionLineEdit->setText(appSettings->value("client_version_xml_path").toString());
    ui->bottleNameLineEdit->setText(appSettings->value("bottle_name", "WindowsGames").toString());
#ifdef Q_OS_WIN
    ui->bottleNameLabel->hide();
    ui->bottleNameLineEdit->hide();
#endif

    //connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::on_saveButtonClicked);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_executableBrowseButton_clicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select WoT Executable", QDir::homePath(), "Executables (*.exe)");
    if (!file.isEmpty()) ui->executableLineEdit->setText(file);
}

void SettingsDialog::on_replaysBrowseButton_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Replays Folder", QDir::homePath());
    if (!dir.isEmpty()) ui->replaysLineEdit->setText(dir);
}

void SettingsDialog::on_versionBrowseButton_clicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select version.xml", QDir::homePath(), "XML Files (*.xml)");
    if (!file.isEmpty()) ui->versionLineEdit->setText(file);
}

void SettingsDialog::on_buttonBox_accepted()
{
    qDebug() << "ButtonBox accepted signal received";
    if (!appSettings) {
        qDebug() << "Settings object is null in accepted slot!";
        return;
    }
    
    saveSettings();
    accept();
}

void SettingsDialog::on_buttonBox_rejected()
{
    qDebug() << "ButtonBox rejected signal received";
    reject();
}

void SettingsDialog::saveSettings()
{
    qDebug() << "Saving settings...";
    if (!appSettings) {
        qDebug() << "Settings object is null!";
        return;
    }
    
    // Cache values before saving
    QString bottleName = ui->bottleNameLineEdit->text().trimmed();
    QString replaysPath = ui->replaysLineEdit->text().trimmed();
    QString executablePath = ui->executableLineEdit->text().trimmed();
    QString versionPath = ui->versionLineEdit->text().trimmed();
    
    qDebug() << "Saving paths:"
             << "\nBottle:" << bottleName
             << "\nReplays:" << replaysPath
             << "\nExecutable:" << executablePath
             << "\nVersion:" << versionPath;
    
    appSettings->setValue("bottle_name", bottleName);
    appSettings->setValue("replays_path", replaysPath);
    appSettings->setValue("executable_path", executablePath);
    appSettings->setValue("client_version_xml_path", versionPath);
    
    appSettings->sync();
    qDebug() << "Settings saved and synced";
}
