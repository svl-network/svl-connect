#include "SVLUpdateManager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>

#include "Application.h"
#include "BuildConfig.h"
#include "SVLSecurity.h"

SVLUpdateManager* SVLUpdateManager::s_instance = nullptr;

SVLUpdateManager* SVLUpdateManager::instance()
{
    if (!s_instance) {
        s_instance = new SVLUpdateManager(APPLICATION);
    }
    return s_instance;
}

SVLUpdateManager::SVLUpdateManager(QObject* parent)
    : QObject(parent)
{
}

void SVLUpdateManager::checkForUpdates(int delayMs)
{
    QTimer::singleShot(delayMs, this, &SVLUpdateManager::onCheckUpdatesTriggered);
}

void SVLUpdateManager::onCheckUpdatesTriggered()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QString masterApiUrl = "http://192.168.0.148:3001";
    QUrl url(QString("%1/api/v1/updates/latest").arg(masterApiUrl));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    req.setTransferTimeout(5000);
    SVLSecurity::injectAuthHeaders(req);

    m_reply = APPLICATION->network()->get(req);
    connect(m_reply, &QNetworkReply::finished, this, &SVLUpdateManager::onUpdateReplyFinished);
}

void SVLUpdateManager::onUpdateReplyFinished()
{
    if (!m_reply) return;

    auto reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[SVLUpdateManager] Update check failed or offline:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "[SVLUpdateManager] Invalid JSON received from /api/v1/updates/latest";
        return;
    }

    QJsonObject root = doc.object();
    if (root.contains("client") && root.value("client").isObject()) {
        QJsonObject clientObj = root.value("client").toObject();
        m_clientInfo.version = clientObj.value("version").toString();
        m_clientInfo.mandatory = clientObj.value("mandatory").toBool(false);
        m_clientInfo.downloadUrl = clientObj.value("url").toString("https://github.com/SunveilNetwork/svl-connect/releases/latest");
        m_clientInfo.changelog = clientObj.value("changelog").toString("New enhancements and performance optimizations.");

        QString localVersion = BuildConfig.printableVersionString();
        if (localVersion.isEmpty()) {
            localVersion = "1.0.0";
        }

        if (isVersionNewer(m_clientInfo.version, localVersion)) {
            qDebug() << "[SVLUpdateManager] Newer version found:" << m_clientInfo.version << "(Local:" << localVersion << ")";
            m_updateAvailable = true;
            emit updateAvailable(m_clientInfo.version, m_clientInfo.mandatory, m_clientInfo.downloadUrl, m_clientInfo.changelog);
        } else {
            qDebug() << "[SVLUpdateManager] Application is up-to-date:" << localVersion;
        }
    }

    if (root.contains("bridge") && root.value("bridge").isObject()) {
        QJsonObject bridgeObj = root.value("bridge").toObject();
        m_bridgeInfo.version = bridgeObj.value("version").toString();
        m_bridgeInfo.downloadUrl = bridgeObj.value("url").toString();
    }
}

bool SVLUpdateManager::isVersionNewer(const QString& remoteVersion, const QString& currentVersion)
{
    if (remoteVersion.isEmpty()) return false;
    if (currentVersion.isEmpty()) return true;

    // Clean version strings (strip suffixes like -main, -dev, v prefix)
    auto cleanVer = [](QString v) -> QStringList {
        if (v.startsWith('v', Qt::CaseInsensitive)) {
            v = v.mid(1);
        }
        int dashIdx = v.indexOf('-');
        if (dashIdx != -1) {
            v = v.left(dashIdx);
        }
        return v.split('.', Qt::SkipEmptyParts);
    };

    QStringList remoteParts = cleanVer(remoteVersion);
    QStringList currentParts = cleanVer(currentVersion);

    int maxLen = qMax(remoteParts.size(), currentParts.size());
    for (int i = 0; i < maxLen; ++i) {
        int rNum = (i < remoteParts.size()) ? remoteParts[i].toInt() : 0;
        int cNum = (i < currentParts.size()) ? currentParts[i].toInt() : 0;

        if (rNum > cNum) return true;
        if (rNum < cNum) return false;
    }

    return false;
}
