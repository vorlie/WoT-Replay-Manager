#include "mainwindow.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QPushButton>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTableWidgetItem>

struct ReplayInfo {
    QString path;
    QString playerName;
    QString tank;
    QString map;
    QString date;
    int damage;
    QString server;
    QString version;
};

QList<ReplayInfo> replaysData;
QMap<QString, QString> tankMap;

MainWIndow::MainWIndow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWIndow)
    , settings(nullptr)
{
    ui->setupUi(this);

    this->setWindowTitle("WoT Replay Manager");
    this->setWindowIcon(QIcon(":/resources/icon.png"));

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);

    QString configPath = configDir + "/config.ini";
    settings = new QSettings(configPath, QSettings::IniFormat);

    wot_executable_path = settings->value("executable_path", "").toString();
    replays_directory = settings->value("replays_path", "").toString();
    bottle_name = settings->value("bottle_name", "WindowsGames").toString();
    client_version_xml_path = settings->value("client_version_xml_path", "").toString();

    ui->replayTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->replayTableWidget->setSortingEnabled(true);
    ui->replayTableWidget->horizontalHeader()->setSectionsClickable(true);

    loadTankMapping(); // load tank JSON mapping

    if (!wot_executable_path.isEmpty() && !replays_directory.isEmpty() && !client_version_xml_path.isEmpty()) {
        loadReplays();
    }
}

MainWIndow::~MainWIndow()
{
    delete ui;
    delete settings;
}

void MainWIndow::loadTankMapping()
{
    QFile file(":/resources/tank_mapping.json"); // QRC resource file
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open tank mapping JSON";
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "Failed to parse tank mapping JSON:" << parseError.errorString();
        return;
    }

    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        tankMap[it.key()] = it.value().toString();
    }

    qDebug() << "Loaded tank mappings:" << tankMap.size();
}

void MainWIndow::on_settingsButton_clicked()
{
    SettingsDialog dlg(settings, this);
    dlg.exec();

    wot_executable_path = settings->value("executable_path", "").toString();
    replays_directory = settings->value("replays_path", "").toString();
    client_version_xml_path = settings->value("client_version_xml_path", "").toString();
    bottle_name = settings->value("bottle_name", "WindowsGames").toString();

    if (!wot_executable_path.isEmpty() && !replays_directory.isEmpty() && !client_version_xml_path.isEmpty()) {
        loadReplays();
    }
}

void MainWIndow::on_cleanupButton_clicked()
{
    QMessageBox::information(this, "Cleanup", "Run replay cleanup here.");
}

void MainWIndow::on_launchButton_clicked()
{
    int row = ui->replayTableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Info", "No replay selected.");
        return;
    }

    QString replayPath = ui->replayTableWidget->item(row, 0)->data(Qt::UserRole).toString();

#ifdef Q_OS_WIN
    if (wot_executable_path.isEmpty()) {
        QMessageBox::critical(this, "Error", "WoT executable path not set.");
        return;
    }
    QProcess::startDetached(wot_executable_path, { replayPath });

#elif defined(Q_OS_LINUX)
    if (wot_executable_path.isEmpty() || bottle_name.isEmpty()) {
        QMessageBox::critical(this, "Error", "WoT executable path or bottle name not set.");
        return;
    }

    QStringList args;
    args << "run" << "-b" << bottle_name << "-e" << wot_executable_path << "--args" << replayPath;
    if (!QProcess::startDetached("bottles-cli", args)) {
        QMessageBox::critical(this, "Error", "Failed to launch replay. Is bottles-cli installed and in PATH?");
    }
#endif
}

void MainWIndow::loadReplays()
{
    ui->replayTableWidget->clearContents();
    ui->replayTableWidget->setRowCount(0);
    ui->replayTableWidget->setColumnCount(7);
    replaysData.clear();

    if (replays_directory.isEmpty()) return;

    // Enable sorting
    ui->replayTableWidget->setSortingEnabled(true);

    // Load tank mappings from resource
    loadTankMapping();

    // Run Python CLI to get replay data
    QString appDir = QCoreApplication::applicationDirPath();
    QString pyCli;
#ifdef Q_OS_WIN
    pyCli = appDir + "/parser/replay_parser.exe";
#else
    pyCli = appDir + "/parser/replay_parser.bin";
#endif
    QStringList args;
    args << replays_directory;

    QProcess process;
    process.start(pyCli, args);
    if (!process.waitForFinished(5000)) {
        QMessageBox::critical(this, "Error", "Python CLI timed out or failed to run.");
        return;
    }

    QByteArray output = process.readAllStandardOutput();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, "Error", "Failed to parse JSON from CLI: " + parseError.errorString());
        return;
    }

    if (!doc.isArray()) {
        QMessageBox::critical(this, "Error", "Unexpected JSON structure from CLI");
        return;
    }

    QJsonArray array = doc.array();
    ui->replayTableWidget->setRowCount(array.size());

    for (int row = 0; row < array.size(); ++row) {
        QJsonValue val = array.at(row);
        if (!val.isObject()) continue;

        QJsonObject obj = val.toObject();
        ReplayInfo info;
        info.path = obj.value("path").toString();
        info.playerName = obj.value("player_name").toString();

        // Extract full tank ID from player_vehicle
        QString fullTankId = obj.value("player_vehicle").toString().split('-').last();
        if (tankMap.contains(fullTankId))
            info.tank = tankMap[fullTankId];
        else
            info.tank = fullTankId;

        info.map = obj.value("map").toString();
        info.date = obj.value("date").toString();
        info.damage = obj.value("damage_dealt").toInt();
        info.server = obj.value("server_name").toString();
        info.version = obj.value("version").toString();

        replaysData.append(info);

        // Populate table items and make read-only
        QTableWidgetItem* item;

        item = new QTableWidgetItem(info.playerName);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info.path); // store path for launching
        ui->replayTableWidget->setItem(row, 0, item);

        item = new QTableWidgetItem(info.tank);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 1, item);

        item = new QTableWidgetItem(info.map);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 2, item);

        item = new QTableWidgetItem(info.date);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 3, item);

        item = new QTableWidgetItem(QString::number(info.damage));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info.damage); // numeric sorting
        ui->replayTableWidget->setItem(row, 4, item);

        item = new QTableWidgetItem(info.server);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 5, item);

        item = new QTableWidgetItem(info.version);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 6, item);
    }

    ui->replayTableWidget->resizeColumnsToContents();
}

