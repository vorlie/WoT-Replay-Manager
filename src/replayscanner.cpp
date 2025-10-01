#include "replayscanner.h"
#include <QDebug>
#include <QFile>
#include <QJsonParseError>
#include <QThread>

extern "C" {
const char* parse_replay(const char* path_to_replay_file);
void free_string(char* s);
}

ReplayScanner::ReplayScanner(const QString& replaysDir, QObject *parent)
    : QObject(parent), m_replaysDirectory(replaysDir)
{
    loadTankMapping();
}

ReplayScanner::~ReplayScanner()
{
}

void ReplayScanner::doScan()
{
    QList<ReplayInfo> newReplaysData;
    QDir dir(m_replaysDirectory);
    QStringList filters;
    filters << "*.wotreplay";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    for (const auto& fileInfo : fileList) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            return;
        }

        emit scanProgress(fileInfo.fileName());

        QString filePath = fileInfo.absoluteFilePath();

        if (m_knownReplayPaths.contains(filePath)) {
            qDebug() << "Skipping known file:" << fileInfo.fileName();
            continue;
        }

        QByteArray filePathBytes = filePath.toUtf8();
        const char* path_c_str = filePathBytes.constData();

        const char* result_c_str = parse_replay(path_c_str);
        if (result_c_str == nullptr) {
            continue;
        }

        QString result_json_str = QString::fromUtf8(result_c_str);
        free_string(const_cast<char*>(result_c_str));

        if (result_json_str.startsWith("Failed to parse replay:") || result_json_str.startsWith("Failed to serialize to JSON:")) {
            continue;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(result_json_str.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        ReplayInfo info;
        info.path = filePath;
        info.playerName = obj.value("playerName").toString();

        QString fullTankStr = obj.value("tank").toString();
        //qDebug() << "Full Tank String" << fullTankStr;
        QString suffixLabel;

        if (fullTankStr.endsWith("_FEP23")) {
            suffixLabel = " (Overwhelming Fire)";
            fullTankStr = fullTankStr.left(fullTankStr.length() - 6);
        }
        //qDebug() << "Suffix Tank Label" << suffixLabel;
        QString tankId = fullTankStr.section('-', 1, -1);
        //qDebug() << "Tank ID" << tankId;
        if (tankMap.contains(tankId)) {
            info.tank = tankMap[tankId] + suffixLabel;
            //qDebug() << "Tank INFO Mapped" << info.tank;
        } else {
            info.tank = fullTankStr + suffixLabel;
            //qDebug() << "Tank INFO" << info.tank;
        }

        info.map = obj.value("map").toString();
        info.date = obj.value("date").toString();
        info.damage = obj.value("damage").toInt();
        info.server = obj.value("server").toString();
        info.version = obj.value("version").toString();

        newReplaysData.append(info);
    }
    emit scanFinished(newReplaysData);
}

void ReplayScanner::loadTankMapping()
{
    QFile file(":/resources/tank_mapping.json");
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
}
