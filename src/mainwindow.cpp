#include "mainwindow.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QMessageBox>
#include <QAbstractItemView>
#include <QPushButton>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTableWidgetItem>
#include <QLabel>
#include <QSet>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QVariant>


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
    // Cleanly stop the background thread if it's running
    if (m_workerThread->isRunning()) {
        m_workerThread->requestInterruption();
        m_workerThread->quit();
        m_workerThread->wait();
    }

    // Close the database connection cleanly
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        QSqlDatabase::database().close();
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
    }

    delete ui;
    delete settings;
}

/**
 * @brief Initializes the SQLite database connection and creates the replays table.
 * @return true if successful, false otherwise.
 */
bool MainWIndow::initializeDatabase()
{
    // Set up a database connection named "qt_sql_default_connection"
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(m_cacheFilePath);

    if (!db.open()) {
        qCritical() << "Error: Failed to connect to database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    // Create the replays table if it doesn't exist.
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS replays ("
            "path TEXT PRIMARY KEY, "
            "playerName TEXT, "
            "tank TEXT, "
            "map TEXT, "
            "date TEXT, "
            "damage INTEGER, "
            "server TEXT, "
            "version TEXT"
            ")"
            )) {
        qCritical() << "Error creating replays table:" << query.lastError().text();
        return false;
    }

    return true;
}

void MainWIndow::setupUiAndConnections()
{
    this->setWindowTitle("WoT Replay Manager");
    this->setWindowIcon(QIcon(":/resources/icon.png"));

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QString configPath = configDir + "/config.ini";

    // Set up SQLite cache file path
    m_cacheFilePath = configDir + "/replays_cache.sqlite";

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

    // Connect the file system watcher
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWIndow::onReplayDirectoryChanged);

    if (initializeDatabase() && !replays_directory.isEmpty()) {
        m_fileWatcher->addPath(replays_directory);
        QSet<QString> knownPaths = loadReplayCache(); // 1. Load existing data for fast display
        startReplayScan(true, knownPaths);  // 2. Start incremental scan to find new files and check for deleted files
    } else if (!replays_directory.isEmpty()){
        statusBar()->showMessage("Database initialization failed. Performing full scan...", 5000);
        startReplayScan(false);
    } else {
        statusBar()->showMessage("Please configure the replay directory in Settings.", 5000);
    }
}

/**
 * @brief Loads replay metadata from the SQLite database and updates the table.
 * @return A QSet of all replay paths currently present in the cache.
 */
QSet<QString> MainWIndow::loadReplayCache()
{
    replaysData.clear(); // Clear the current in-memory list
    QSet<QString> cachePaths; // To store paths currently in the DB

    QSqlQuery query("SELECT path, playerName, tank, map, date, damage, server, version FROM replays");

    if (!query.exec()) {
        qCritical() << "Error selecting replays:" << query.lastError().text();
        return cachePaths;
    }

    while (query.next()) {
        ReplayInfo info;
        // Map query result columns to ReplayInfo members
        info.path = query.value(0).toString();
        info.playerName = query.value(1).toString();
        info.tank = query.value(2).toString();
        info.map = query.value(3).toString();
        info.date = query.value(4).toString();
        info.damage = query.value(5).toInt(); // Ensure damage is read as an integer
        info.server = query.value(6).toString();
        info.version = query.value(7).toString();

        // Only load data into the table if the file still exists on disk
        if (QFile::exists(info.path)) {
            replaysData.append(info);
        }

        // Always add to cachePaths set, even if the file is deleted (we need this to detect stale entries)
        cachePaths.insert(info.path);
    }

    updateTable(replaysData);
    statusBar()->showMessage("Loaded " + QString::number(replaysData.size()) + " replays from cache.", 3000);
    return cachePaths;
}

/**
 * @brief Deletes replay entries from the database whose files no longer exist on disk.
 * @param pathsToDelete A QSet of absolute file paths to remove from the DB.
 */
void MainWIndow::deleteStaleReplayCacheEntries(const QSet<QString>& pathsToDelete)
{
    if (pathsToDelete.isEmpty()) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qWarning() << "Database not open for deleting stale entries.";
        return;
    }

    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for cleanup:" << db.lastError().text();
        return;
    }

    QSqlQuery query;
    QString placeholders = pathsToDelete.values().join("','");
    QString deleteQuery = QString("DELETE FROM replays WHERE path IN ('%1')").arg(placeholders);

    if (query.exec(deleteQuery)) {
        if (db.commit()) {
            qDebug() << "Successfully deleted" << pathsToDelete.size() << "stale entries from cache.";
        } else {
            qCritical() << "Failed to commit cleanup transaction:" << db.lastError().text();
            db.rollback();
        }
    } else {
        qCritical() << "Error executing delete query for stale entries:" << query.lastError().text();
        db.rollback();
    }
}

/**
 * @brief Inserts or updates replay metadata into the SQLite database using a transaction.
 * @param replays The list of new/updated replay data to save.
 */
void MainWIndow::saveReplayCache(const QList<ReplayInfo>& replays)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qWarning() << "Database not open for saving.";
        return;
    }

    // Start transaction for speed and atomicity
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction:" << db.lastError().text();
        return;
    }

    QSqlQuery query;
    // Uses INSERT OR REPLACE INTO: if a replay with the same path (PRIMARY KEY) exists, it updates it.
    query.prepare(
        "INSERT OR REPLACE INTO replays (path, playerName, tank, map, date, damage, server, version) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

    for (const auto& info : replays) {
        query.bindValue(0, info.path);
        query.bindValue(1, info.playerName);
        query.bindValue(2, info.tank);
        query.bindValue(3, info.map);
        query.bindValue(4, info.date);
        query.bindValue(5, info.damage);
        query.bindValue(6, info.server);
        query.bindValue(7, info.version);
        if (!query.exec()) {
            qCritical() << "Error inserting/replacing replay:" << query.lastError().text();
            db.rollback();
            return;
        }
    }

    if (db.commit()) {
        qDebug() << "Replay cache updated successfully with" << replays.size() << "new/updated entries.";
    } else {
        qCritical() << "Failed to commit transaction:" << db.lastError().text();
        db.rollback();
    }
}

void MainWIndow::startReplayScan(bool incremental, const QSet<QString>& knownPaths)
{
    if (m_workerThread->isRunning()) {
        qDebug() << "Scan already in progress. Ignoring new request.";
        return;
    }

    // Cleanly stop any previous thread execution
    m_workerThread->quit();
    m_workerThread->wait();

    if (m_scanner) {
        m_scanner->disconnect();
        m_scanner->deleteLater();
        m_scanner = nullptr;
    }

    if (replays_directory.isEmpty()) {
        qDebug() << "Replays directory is empty! Cannot start scan.";
        return;
    }

    // Create a new scanner instance
    m_scanner = new ReplayScanner(replays_directory);

    if (incremental) {
        m_scanner->setKnownReplayPaths(knownPaths);
        statusBar()->showMessage("Starting incremental scan for new files...");
    } else {
        m_scanner->setKnownReplayPaths({});
        statusBar()->showMessage("Starting full scan (parsing all files)...");
    }

    if (!m_scanner) {
        qDebug() << "Failed to create scanner!";
        return;
    }

    // Move scanner worker to the thread
    m_scanner->moveToThread(m_workerThread);

    // Connect signals
    connect(m_workerThread, &QThread::started, m_scanner, &ReplayScanner::doScan, Qt::QueuedConnection);
    connect(m_scanner, &ReplayScanner::scanFinished, this, &MainWIndow::onReplayScanFinished, Qt::QueuedConnection);
    connect(m_scanner, &ReplayScanner::scanProgress, this, &MainWIndow::onReplayScanProgress, Qt::QueuedConnection);
    connect(m_scanner, &ReplayScanner::scanFinished, m_workerThread, &QThread::quit, Qt::QueuedConnection);
    connect(m_workerThread, &QThread::finished, this, [this]() {
        // Clean up the scanner object when the thread finishes
        if (m_scanner) {
            m_scanner->deleteLater();
            m_scanner = nullptr;
        }
    }, Qt::QueuedConnection);

    m_workerThread->start();
}

void MainWIndow::onReplayScanFinished(const QList<ReplayInfo>& newReplays)
{
    qDebug() << "Received scan results, new/updated replays found:" << newReplays.size();

    if (!newReplays.isEmpty()) {
        // 1. Save the newly scanned replays (will INSERT or REPLACE existing entries)
        saveReplayCache(newReplays);
    }

    // 2. Perform file system synchronization to detect deleted files
    QDir dir(replays_directory);
    QStringList filters;
    filters << "*.wotreplay";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    QSet<QString> allDiskPaths;
    for (const auto& fileInfo : fileList) {
        allDiskPaths.insert(fileInfo.absoluteFilePath());
    }

    // Get all paths currently stored in the cache
    QSet<QString> allCachePaths = loadReplayCache();

    QSet<QString> pathsToDelete;
    for (const QString& path : allCachePaths) {
        // If a path exists in the cache but NOT on disk, it needs to be deleted from the cache
        if (!allDiskPaths.contains(path)) {
            pathsToDelete.insert(path);
        }
    }

    // 3. Delete stale entries from the database
    deleteStaleReplayCacheEntries(pathsToDelete);

    // 4. Reload the table after sync
    loadReplayCache();

    statusBar()->showMessage("Scan and synchronization complete! Found " + QString::number(replaysData.size()) + " total replays.", 5000);
}

void MainWIndow::onReplayScanProgress(const QString& currentFile)
{
    statusBar()->showMessage("Scanning: " + currentFile);
}

void MainWIndow::onReplayDirectoryChanged(const QString& path)
{
    qDebug() << "Replay directory content changed in:" << path;
    startReplayScan(true);
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

        // Column 0: Player Name
        item = new QTableWidgetItem(info.playerName);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        // Store the full replay path in the UserRole for launching the replay later
        item->setData(Qt::UserRole, info.path);
        ui->replayTableWidget->setItem(row, 0, item);

        // Column 1: Tank
        item = new QTableWidgetItem(info.tank);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 1, item);

        // Column 2: Map
        item = new QTableWidgetItem(info.map);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 2, item);

        // Column 3: Date
        item = new QTableWidgetItem(info.date);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 3, item);

        // Column 4: Damage
        item = new DamageTableWidgetItem();
        item->setText(QString::number(info.damage));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info.damage);
        ui->replayTableWidget->setItem(row, 4, item);

        // Column 5: Server
        item = new QTableWidgetItem(info.server);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 5, item);

        // Column 6: Version
        item = new QTableWidgetItem(info.version);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->replayTableWidget->setItem(row, 6, item);
    }
    ui->replayTableWidget->resizeColumnsToContents();
}

void MainWIndow::on_replayTableWidget_itemSelectionChanged()
{
    bool hasSelection = !ui->replayTableWidget->selectedItems().isEmpty();
    // Enable/disable the launch button based on selection
    ui->launchButton->setEnabled(hasSelection);
}

void MainWIndow::on_settingsButton_clicked()
{
    if (!settings) {
        qDebug() << "Settings object is null!";
        return;
    }

    SettingsDialog dlg(settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString oldReplaysDirectory = replays_directory;

        settings->sync();

        // Read settings with fallbacks
        wot_executable_path = settings->value("executable_path", wot_executable_path).toString();
        replays_directory = settings->value("replays_path", replays_directory).toString();
        client_version_xml_path = settings->value("client_version_xml_path", client_version_xml_path).toString();
        bottle_name = settings->value("bottle_name", bottle_name).toString();

        // Check if the directory path has changed
        if (!replays_directory.isEmpty() && oldReplaysDirectory != replays_directory) {
            // Remove the old path from the file watcher to prevent crashes
            if (!oldReplaysDirectory.isEmpty() &&
                m_fileWatcher->directories().contains(oldReplaysDirectory)) {
                m_fileWatcher->removePath(oldReplaysDirectory);
            }

            // Add the new path to the file watcher
            if (!m_fileWatcher->addPath(replays_directory)) {
                qDebug() << "Failed to add new path to file watcher:" << replays_directory;
            }

            // If the directory changes, trigger a full scan (DB content might be irrelevant now)
            startReplayScan(false);
        } else if (!replays_directory.isEmpty()) {
            // Directory didn't change, just ensure the current view is loaded and check for new files
            QSet<QString> knownPaths = loadReplayCache();
            startReplayScan(true, knownPaths);
        }
    }
}

void MainWIndow::on_cleanupButton_clicked()
{
    // Re-use the synchronization logic from the scan finished handler
    // This provides a manual way to sync the cache if needed

    // 1. Get all file paths on disk
    QDir dir(replays_directory);
    QStringList filters;
    filters << "*.wotreplay";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    QSet<QString> allDiskPaths;
    for (const auto& fileInfo : fileList) {
        allDiskPaths.insert(fileInfo.absoluteFilePath());
    }

    // 2. Get all paths currently stored in the cache
    QSet<QString> allCachePaths = loadReplayCache();

    QSet<QString> pathsToDelete;
    for (const QString& path : allCachePaths) {
        // If a path exists in the cache but NOT on disk, it needs to be deleted from the cache
        if (!allDiskPaths.contains(path)) {
            pathsToDelete.insert(path);
        }
    }

    // 3. Delete stale entries
    if (pathsToDelete.size() > 0) {
        deleteStaleReplayCacheEntries(pathsToDelete);
        // 4. Reload the table to reflect deletions
        loadReplayCache();
        QMessageBox::information(this, "Cleanup Complete", QString("Removed %1 entries from the database that no longer exist on disk.").arg(pathsToDelete.size()));
    } else {
        QMessageBox::information(this, "Cleanup Complete", "No stale entries found in the database.");
    }
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
