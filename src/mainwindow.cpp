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
#include <QLabel>

extern "C" {
const char* parse_replay(const char* path_to_replay_file);
void free_string(char* s);
}

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

    QLabel* reportLabel = new QLabel(
        "<a href='https://github.com/vorlie/WoT-Replay-Manager/issues/new?template=tank_mapping.yml'>Report Incorrect Tank Name</a>", this);
    reportLabel->setTextFormat(Qt::RichText);
    reportLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    reportLabel->setOpenExternalLinks(true);

    QLabel* aboutLabel = new QLabel(
        "<a href='https://github.com/vorlie/WoT-Replay-Manager?tab=readme-ov-file'>Repository</a>", this);
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    aboutLabel->setOpenExternalLinks(true);

    statusBar()->addWidget(aboutLabel);
    statusBar()->addWidget(reportLabel);

    reportLabel->setContentsMargins(5, 0, 0, 5);
    aboutLabel->setContentsMargins(10, 0, 0, 5);

    loadTankMapping(); // load tank JSON mapping

    if (!wot_executable_path.isEmpty() && !replays_directory.isEmpty() && !client_version_xml_path.isEmpty()) {
        loadReplays();
    }

    ui->replayTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->replayTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // Initially disable Launch button
    ui->launchButton->setEnabled(false);

    // Connect selection change → enable/disable launch button
    connect(ui->replayTableWidget, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !ui->replayTableWidget->selectedItems().isEmpty();
        ui->launchButton->setEnabled(hasSelection);
    });

    ui->launchButton->setStyleSheet(R"(
        QPushButton:enabled {
            background-color: #3A6D99;
            color: #FFFFFF;
        }
        QPushButton:hover:enabled {
            background-color: #335c85;
        }
    )");
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

    // Set the horizontal headers (table names)
    QStringList labels;
    labels << "Player" << "Tank" << "Map" << "Date" << "Damage" << "Server" << "Version";
    ui->replayTableWidget->setHorizontalHeaderLabels(labels);

    if (replays_directory.isEmpty()) return;

    // Enable sorting
    ui->replayTableWidget->setSortingEnabled(true);

    // Load tank mappings from resource
    loadTankMapping();

    // Loop through all .wotreplay files in the directory
    QDir dir(replays_directory);
    QStringList filters;
    filters << "*.wotreplay";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    ui->replayTableWidget->setRowCount(fileList.size());

    for (int row = 0; row < fileList.size(); ++row) {
        QFileInfo fileInfo = fileList.at(row);
        QString filePath = fileInfo.absoluteFilePath();

        // Convert QString to C-style string for FFI
        QByteArray filePathBytes = filePath.toUtf8();
        const char* path_c_str = filePathBytes.constData();

        // Call the Rust function
        const char* result_c_str = parse_replay(path_c_str);

        if (result_c_str == nullptr) {
            QMessageBox::critical(this, "Error", "Rust parser returned a null pointer.");
            continue;
        }

        // Convert the result back to QString
        QString result_json_str = QString::fromUtf8(result_c_str);

        // Check for an error message from Rust
        if (result_json_str.startsWith("Failed to parse replay:") || result_json_str.startsWith("Failed to serialize to JSON:")) {
            //qDebug() << "Rust Error for" << filePath << ":" << result_json_str;
            free_string(const_cast<char*>(result_c_str));
            continue;
        }

        // Parse the JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(result_json_str.toUtf8(), &parseError);

        // Release the memory from the Rust side
        free_string(const_cast<char*>(result_c_str));

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            //qDebug() << "Failed to parse JSON from Rust: " << parseError.errorString() << "for file:" << filePath;
            continue;
        }

        QJsonObject obj = doc.object();
        ReplayInfo info;
        info.path = obj.value("path").toString();
        info.playerName = obj.value("playerName").toString();
        qDebug().noquote() << "[DEBUG] Original tank string:" << obj.value("tank").toString();
        qDebug().noquote() << "[DEBUG] Split tank ID:" << obj.value("tank").toString().split('-').last();

        QString fullTankStr = obj.value("tank").toString();  // "ussr-R47_ISU-152"
        QString tankId = fullTankStr.section('-', 1, -1);   // "R47_ISU-152"
        // Split off nation part
        QStringList parts = fullTankStr.split('-', Qt::SkipEmptyParts);
        QString suffixLabel;

        // Check if there's a special event suffix
        if (fullTankStr.endsWith("_FEP23")) {
            suffixLabel = " (Overwhelming Fire)";
            // Remove the suffix before looking up the map
            tankId = fullTankStr.left(fullTankStr.length() - 6).section('-', 1, -1);
        } else {
            tankId = fullTankStr.section('-', 1, -1);
        }

        // Lookup in the tank map
        if (tankMap.contains(tankId))
            info.tank = tankMap[tankId] + suffixLabel;
        else
            info.tank = fullTankStr + suffixLabel;

        qDebug().noquote() << "[DEBUG] Resolved tank name:" << info.tank;
        info.map = obj.value("map").toString();
        info.date = obj.value("date").toString();
        info.damage = obj.value("damage").toInt();
        info.server = obj.value("server").toString();
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
