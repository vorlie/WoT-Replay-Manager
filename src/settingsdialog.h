#include <QDialog>
#include <QSettings>

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
    void on_saveButton_clicked();

private:
    Ui::SettingsDialog *ui;
    QSettings *appSettings;
};
