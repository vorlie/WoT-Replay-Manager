#ifndef REPLAYSCANNER_H
#define REPLAYSCANNER_H

#include <QObject>
#include <QDir>
#include <QSet>
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

    void setKnownReplayPaths(const QSet<QString>& knownPaths) {
        m_knownReplayPaths = knownPaths;
    }


public slots:
    void doScan();

signals:
    void scanFinished(const QList<ReplayInfo>& replays);
    void scanProgress(const QString& currentFile);

private:
    void loadTankMapping();

    QString m_replaysDirectory;
    QMap<QString, QString> tankMap;

    QSet<QString> m_knownReplayPaths;
};

#endif // REPLAYSCANNER_H
