#ifndef REPLAYSCANNER_H
#define REPLAYSCANNER_H

#include <QObject>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

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

class ReplayScanner : public QObject
{
    Q_OBJECT
public:
    explicit ReplayScanner(const QString& replaysDir, QObject *parent = nullptr);
    ~ReplayScanner();

signals:
    void scanFinished(const QList<ReplayInfo>& replays);
    void scanProgress(const QString& currentFile);

public slots:
    void doScan();

private:
    void loadTankMapping();

    QString m_replaysDirectory;
    QMap<QString, QString> tankMap;
};

#endif // REPLAYSCANNER_H
