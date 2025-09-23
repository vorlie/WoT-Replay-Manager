#include "mainwindow.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QPushButton>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTableWidgetItem>
#include <QLabel>
#include <QSet>

MainWIndow::MainWIndow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWIndow)
    , settings(nullptr)
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_workerThread(new QThread(this))
    , m_scanner(nullptr)
{
    ui->setupUi(this);
    setupUiAndConnections();
}

MainWIndow::~MainWIndow()
{
    if (m_workerThread->isRunning()) {
        m_workerThread->requestInterruption();
        m_workerThread->quit();
        m_workerThread->wait();
    }
    delete ui;
    delete settings;
}

void MainWIndow::setupUiAndConnections()
{
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
    ui->replayTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->replayTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->replayTableWidget->setColumnCount(7);
    ui->launchButton->setEnabled(false);

    QLabel* reportLabel = new QLabel("<a href='https://github.com/vorlie/WoT-Replay-Manager/issues/new?template=tank_mapping.yml'>Report Incorrect Tank Name</a>", this);
    reportLabel->setTextFormat(Qt::RichText);
    reportLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    reportLabel->setOpenExternalLinks(true);
    QLabel* aboutLabel = new QLabel("<a href='https://github.com/vorlie/WoT-Replay-Manager?tab=readme-ov-file'>Repository</a>", this);
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    aboutLabel->setOpenExternalLinks(true);
    statusBar()->addWidget(aboutLabel);
    statusBar()->addWidget(reportLabel);
    reportLabel->setContentsMargins(5, 0, 0, 5);
    aboutLabel->setContentsMargins(10, 0, 0, 5);

    // Connect signals for the main UI
    connect(ui->replayTableWidget, &QTableWidget::itemSelectionChanged, this, &MainWIndow::on_replayTableWidget_itemSelectionChanged);
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWIndow::on_settingsButton_clicked);
    connect(ui->cleanupButton, &QPushButton::clicked, this, &MainWIndow::on_cleanupButton_clicked);
    connect(ui->launchButton, &QPushButton::clicked, this, &MainWIndow::on_launchButton_clicked);

    // Connect the file system watcher
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWIndow::onReplayDirectoryChanged);

    // Initial scan on startup
    if (!replays_directory.isEmpty()) {
        m_fileWatcher->addPath(replays_directory);
        startReplayScan();
    }
}

void MainWIndow::startReplayScan(bool incremental)
{
    if (m_workerThread->isRunning()) {
        qDebug() << "Scan already in progress.";
        return;
    }

    if (m_scanner) {
        m_scanner->deleteLater();
        m_scanner = nullptr;
    }

    m_scanner = new ReplayScanner(replays_directory);
    m_scanner->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_scanner, &ReplayScanner::doScan);
    connect(m_scanner, &ReplayScanner::scanFinished, this, &MainWIndow::onReplayScanFinished);
    connect(m_scanner, &ReplayScanner::scanProgress, this, &MainWIndow::onReplayScanProgress);
    connect(m_scanner, &ReplayScanner::scanFinished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_scanner, &QObject::deleteLater);

    m_workerThread->start();
}

void MainWIndow::onReplayScanFinished(const QList<ReplayInfo>& replays)
{
    qDebug() << "Received scan results, total replays:" << replays.size();
    replaysData = replays;
    updateTable(replaysData);
    statusBar()->showMessage("Scan complete! Found " + QString::number(replays.size()) + " replays.", 5000);
}

void MainWIndow::onReplayScanProgress(const QString& currentFile)
{
    statusBar()->showMessage("Scanning: " + currentFile);
}

void MainWIndow::onReplayDirectoryChanged(const QString& path)
{
    qDebug() << "Replay directory changed:" << path;
    startReplayScan();
}

void MainWIndow::updateTable(const QList<ReplayInfo>& replays)
{
    ui->replayTableWidget->clearContents();
    ui->replayTableWidget->setRowCount(replays.size());

    QStringList labels;
    labels << "Player" << "Tank" << "Map" << "Date" << "Damage" << "Server" << "Version";
    ui->replayTableWidget->setHorizontalHeaderLabels(labels);

    for (int row = 0; row < replays.size(); ++row) {
        const ReplayInfo& info = replays[row];
        QTableWidgetItem* item;

        item = new QTableWidgetItem(info.playerName);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info.path);
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
        item->setData(Qt::UserRole, info.damage);
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

void MainWIndow::on_replayTableWidget_itemSelectionChanged()
{
    bool hasSelection = !ui->replayTableWidget->selectedItems().isEmpty();
    ui->launchButton->setEnabled(hasSelection);
}

void MainWIndow::on_settingsButton_clicked()
{
    SettingsDialog dlg(settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        wot_executable_path = settings->value("executable_path", "").toString();
        replays_directory = settings->value("replays_path", "").toString();
        client_version_xml_path = settings->value("client_version_xml_path", "").toString();
        bottle_name = settings->value("bottle_name", "WindowsGames").toString();

        if (!replays_directory.isEmpty()) {
            if (!m_fileWatcher->directories().contains(replays_directory)) {
                m_fileWatcher->addPath(replays_directory);
            }
            startReplayScan();
        }
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

    QTableWidgetItem* item = ui->replayTableWidget->item(row, 0);
    if (!item) {
        return;
    }

    QString replayPath = item->data(Qt::UserRole).toString();

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
