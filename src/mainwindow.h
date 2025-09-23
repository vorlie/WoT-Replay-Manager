#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QTableWidget>
#include <QFileSystemWatcher>
#include <QThread>
#include <QHash>
#include "replayscanner.h"

namespace Ui { class MainWIndow; }

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
    void startReplayScan(bool incremental = false);
    void updateTable(const QList<ReplayInfo>& replays);

    // Configuration and data members
    QSettings *settings;
    QString wot_executable_path;
    QString replays_directory;
    QString bottle_name;
    QString client_version_xml_path;
    QList<ReplayInfo> replaysData;
};

#endif // MAINWINDOW_H
