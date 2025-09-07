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

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::on_saveButton_clicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
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

void SettingsDialog::on_saveButton_clicked() {
    appSettings->setValue("bottle_name", ui->bottleNameLineEdit->text());
    appSettings->setValue("replays_path", ui->replaysLineEdit->text());
    appSettings->setValue("executable_path", ui->executableLineEdit->text());
    appSettings->setValue("client_version_xml_path", ui->versionLineEdit->text());

    appSettings->sync();

    accept();
}
