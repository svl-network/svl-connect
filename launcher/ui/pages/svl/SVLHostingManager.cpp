#include "SVLHostingManager.h"
#include "tasks/SVLBridgeInjector.h"
#include "minecraft/MinecraftInstance.h"
#include "SVLSecurity.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QDebug>

SVLHostingManager::SVLHostingManager(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent), m_nam(nam) {}

bool SVLHostingManager::prepareP2PHosting(MinecraftInstance* instance, const QString& worldName, int maxPlayers, QString* shareCodeOut) {
    if (!instance) {
        emit hostingError("No Minecraft instance selected.");
        return false;
    }

    const QString shareCode = SVLBridgeInjector::generateFriendlyShareCode();

    BridgeConfigOptions options;
    options.serverKey = shareCode;
    options.serverName = QString("%1's World (%2)").arg(instance->name(), worldName.isEmpty() ? "P2P" : worldName);
    options.tunnelEnabled = true;
    options.localServerPort = 25565;

    QString error;
    if (!SVLBridgeInjector::injectBridge(instance, options, &error)) {
        emit hostingError(QString("Failed to configure P2P tunnel bridge: %1").arg(error));
        return false;
    }

    if (shareCodeOut) {
        *shareCodeOut = shareCode;
    }

    const QString inviteLink = QString("svl://join?code=%1&host=realms.sunveil.net:25565").arg(shareCode);
    emit p2pShareCodeReady(shareCode, inviteLink);
    return true;
}

void SVLHostingManager::requestCloudServerCreation(const QString& serverName, const QString& requestedSubdomain, const QString& authToken) {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    QUrl url(m_masterApiBaseUrl + "/api/v1/user/servers/create");
    QNetworkRequest request(url);
    SVLSecurity::injectAuthHeaders(request);

    if (!authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());
    }

    QJsonObject payload;
    payload["name"] = serverName;
    if (!requestedSubdomain.trimmed().isEmpty()) {
        payload["serverKey"] = requestedSubdomain.trimmed().toLower();
    }

    m_currentReply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &SVLHostingManager::onCloudCreateReplyFinished);
}

void SVLHostingManager::onCloudCreateReplyFinished() {
    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errBody = QString::fromUtf8(reply->readAll());
        emit hostingError(QString("Cloud Server creation failed: %1").arg(errBody.isEmpty() ? reply->errorString() : errBody));
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();

    CloudServerCreationResult result;
    result.success = obj.value("success").toBool(true);
    result.serverKey = obj.value("serverKey").toString();
    result.serverName = obj.value("name").toString();
    result.ip = obj.value("ip").toString("realms.sunveil.net");
    result.port = static_cast<quint16>(obj.value("port").toInt(25565));
    result.licenseKey = obj.value("licenseKey").toString();

    emit cloudServerCreated(result);
}
