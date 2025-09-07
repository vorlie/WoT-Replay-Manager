#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>

// Forward declaration of the Ui namespace
// This is created by Qt's build system from your .ui file.
namespace Ui {
class MainWIndow;
}

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

private:
    // Pointer to the UI object. It's how we access the widgets.
    Ui::MainWIndow *ui;

    QSettings *settings;
    QString wot_executable_path;
    QString replays_directory;
    QString bottle_name;
    QString client_version_xml_path;
    QHash<QString, QString> tankMap;   // tag -> short_name mapping

    void loadReplays();
    void loadTankMapping();
};

#endif // MAINWINDOW_H
