#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTableWidget>
#include <QFileSystemWatcher>
#include <QThread>
#include <QHash>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QSet>
#include "replayscanner.h"

namespace Ui { class MainWIndow; }

class DamageTableWidgetItem : public QTableWidgetItem {
public:
    bool operator <(const QTableWidgetItem &other) const override
    {
        // Compare the raw integer data stored in Qt::UserRole for accurate numerical sorting.
        return data(Qt::UserRole).toInt() < other.data(Qt::UserRole).toInt();
    }
};

class MainWIndow : public QMainWindow
{
    Q_OBJECT

public:
    // Constructor and destructor.
    explicit MainWIndow(QWidget *parent = nullptr);
    ~MainWIndow();

private slots:
    // Slots to handle button clicks.
    void on_settingsButton_clicked();
    void on_cleanupButton_clicked();
    void on_launchButton_clicked();
    void on_replayTableWidget_itemSelectionChanged();
    void onReplayScanFinished(const QList<ReplayInfo>& replays);
    void onReplayScanProgress(const QString& currentFile);
    void onReplayDirectoryChanged(const QString& path);
    void setupUiAndConnections();

private:
    // Pointer to the UI object. It's how we access the widgets.
    Ui::MainWIndow *ui;

    // Member variables for threading and file watching
    QFileSystemWatcher* m_fileWatcher;
    QThread* m_workerThread;
    ReplayScanner* m_scanner;

    // Private methods for scan and table management
    void startReplayScan(bool incremental = false, const QSet<QString>& knownPaths = {});
    void updateTable(const QList<ReplayInfo>& replays);

    // Private methods for database management
    bool initializeDatabase();
    QSet<QString> loadReplayCache();
    void saveReplayCache(const QList<ReplayInfo>& replays);
    void deleteStaleReplayCacheEntries(const QSet<QString>& pathsToDelete);

    // Configuration and data members
    QSettings *settings;
    QString wot_executable_path;
    QString replays_directory;
    QString bottle_name;
    QString client_version_xml_path;
    QList<ReplayInfo> replaysData;
    QString m_cacheFilePath;
};

#endif // MAINWINDOW_H
