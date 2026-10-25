#include "SVLFriendManager.h"
#include "SVLSecurity.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QDebug>

SVLFriendManager::SVLFriendManager(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent), m_nam(nam) {
    connect(&m_pollTimer, &QTimer::timeout, this, &SVLFriendManager::pollStatus);
}

void SVLFriendManager::startPolling(int intervalMs) {
    m_pollTimer.start(intervalMs);
    pollStatus();
}

void SVLFriendManager::stopPolling() {
    m_pollTimer.stop();
}

void SVLFriendManager::refreshFriends() {
    pollStatus();
}

bool SVLFriendManager::validateUsername(const QString& username) const {
    static const QRegularExpression regex("^[a-zA-Z0-9_]{3,16}$");
    return regex.match(username.trimmed()).hasMatch();
}

bool SVLFriendManager::validateAddress(const QString& address) const {
    const QString clean = address.trimmed().toLower();
    if (clean.isEmpty() || clean.length() > 253) return false;

    // Check valid hostname or IP characters
    static const QRegularExpression fqdnRegex("^[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(\\.[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$");
    if (!fqdnRegex.match(clean).hasMatch()) return false;

    // Reject dangerous loopback / private IP ranges unless Sunveil realm domain
    if (clean == "127.0.0.1" || clean == "localhost" || clean.startsWith("169.254.") || clean.startsWith("0.0.0.0")) {
        return false;
    }

    return true;
}

void SVLFriendManager::pollStatus() {
    if (m_pollReply) return;

    QUrl url(m_masterApiUrl + "/api/v1/friends/status");
    QNetworkRequest request(url);
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    m_pollReply = m_nam->get(request);
    connect(m_pollReply, &QNetworkReply::finished, this, &SVLFriendManager::onFriendsReplyFinished);
}

void SVLFriendManager::onFriendsReplyFinished() {
    if (!m_pollReply) return;
    QNetworkReply* reply = m_pollReply;
    m_pollReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonObject root = doc.object();

    // 1. Parse Friends List
    const QJsonArray friendsArray = root.value("friends").toArray();
    m_friends.clear();
    for (const auto& val : friendsArray) {
        const QJsonObject fObj = val.toObject();
        FriendStatusModel f;
        f.username = fObj.value("username").toString();
        f.uuid = fObj.value("userId").toString();

        const QString stateStr = fObj.value("state").toString("Offline");
        if (stateStr == "InSingleplayerP2P" || stateStr == "P2P") f.state = FriendActivityState::InSingleplayerP2P;
        else if (stateStr == "OnCloudServer" || stateStr == "CloudServer") f.state = FriendActivityState::OnCloudServer;
        else if (stateStr == "OnSunveilSMP" || stateStr == "SunveilSMP") f.state = FriendActivityState::OnSunveilSMP;
        else if (stateStr == "Online") f.state = FriendActivityState::OnlineInLauncher;
        else f.state = FriendActivityState::Offline;

        f.activityDetails = fObj.value("activityDetails").toString();
        const QString rawAddr = fObj.value("targetAddress").toString();
        f.targetAddress = validateAddress(rawAddr) ? rawAddr : QString();
        f.targetPort = static_cast<quint16>(fObj.value("targetPort").toInt(25565));
        f.avatarUrl = fObj.value("avatarUrl").toString();

        m_friends.append(f);
    }
    emit friendsUpdated(m_friends);

    // 2. Parse Incoming & Outgoing Requests
    const QJsonArray inArray = root.value("incomingRequests").toArray();
    m_incomingRequests.clear();
    for (const auto& val : inArray) {
        const QJsonObject rObj = val.toObject();
        FriendRequestModel req;
        req.userId = rObj.value("userId").toString();
        req.username = rObj.value("username").toString();
        req.isIncoming = true;
        m_incomingRequests.append(req);
    }

    const QJsonArray outArray = root.value("outgoingRequests").toArray();
    m_outgoingRequests.clear();
    for (const auto& val : outArray) {
        const QJsonObject rObj = val.toObject();
        FriendRequestModel req;
        req.userId = rObj.value("userId").toString();
        req.username = rObj.value("username").toString();
        req.isIncoming = false;
        m_outgoingRequests.append(req);
    }
    emit requestsUpdated(m_incomingRequests, m_outgoingRequests);

    // 3. Parse Pending Game Invites
    const QJsonArray invitesArray = root.value("pendingInvites").toArray();
    m_pendingInvites.clear();
    for (const auto& val : invitesArray) {
        const QJsonObject iObj = val.toObject();
        GameInviteModel inv;
        inv.inviteId = iObj.value("inviteId").toString();
        inv.fromUserId = iObj.value("fromUserId").toString();
        inv.fromUsername = iObj.value("fromUsername").toString();
        inv.serverName = iObj.value("serverName").toString();
        inv.targetAddress = iObj.value("targetAddress").toString();
        inv.targetPort = static_cast<quint16>(iObj.value("targetPort").toInt(25565));
        inv.timestamp = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(iObj.value("timestamp").toDouble()));
        inv.expiresAt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(iObj.value("expiresAt").toDouble()));

        m_pendingInvites.append(inv);

        // Notify user once per unique invite
        if (!m_notifiedInviteIds.contains(inv.inviteId)) {
            m_notifiedInviteIds.insert(inv.inviteId);
            emit gameInviteReceived(inv);
        }
    }
}

void SVLFriendManager::postSimpleAction(const QString& endpoint, const QString& targetUsername, const QString& successMsg) {
    if (!validateUsername(targetUsername)) {
        emit friendActionFailed(tr("Invalid Minecraft username format (3-16 alphanumeric characters)."));
        return;
    }

    QUrl url(m_masterApiUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject payload;
    payload["targetUsername"] = targetUsername.trimmed();

    auto* reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, successMsg]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const QByteArray errData = reply->readAll();
            const QJsonDocument errDoc = QJsonDocument::fromJson(errData);
            const QString msg = errDoc.object().value("message").toString(reply->errorString());
            emit friendActionFailed(msg);
            return;
        }

        if (!successMsg.isEmpty()) {
            emit friendActionSucceeded(successMsg);
        }
        pollStatus();
    });
}

void SVLFriendManager::addFriend(const QString& username) {
    postSimpleAction("/api/v1/friends/add", username, tr("Friend request sent to %1!").arg(username));
}

void SVLFriendManager::acceptFriend(const QString& username) {
    postSimpleAction("/api/v1/friends/accept", username, tr("Accepted friend request from %1!").arg(username));
}

void SVLFriendManager::declineFriend(const QString& username) {
    postSimpleAction("/api/v1/friends/decline", username);
}

void SVLFriendManager::removeFriend(const QString& username) {
    postSimpleAction("/api/v1/friends/remove", username);
}

void SVLFriendManager::blockUser(const QString& username) {
    postSimpleAction("/api/v1/friends/block", username, tr("%1 has been blocked.").arg(username));
}

void SVLFriendManager::unblockUser(const QString& username) {
    postSimpleAction("/api/v1/friends/unblock", username, tr("%1 has been unblocked.").arg(username));
}

void SVLFriendManager::sendChatMessage(const QString& targetUsername, const QString& text) {
    const QString cleanText = text.trimmed();
    if (cleanText.isEmpty()) return;
    if (cleanText.length() > 400) {
        emit friendActionFailed(tr("Message is too long (maximum 400 characters)."));
        return;
    }
    if (!validateUsername(targetUsername)) {
        emit friendActionFailed(tr("Invalid recipient username."));
        return;
    }

    QUrl url(m_masterApiUrl + "/api/v1/friends/chat");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject payload;
    payload["targetUsername"] = targetUsername.trimmed();
    payload["text"] = cleanText;

    auto* reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const QByteArray errData = reply->readAll();
            const QJsonDocument errDoc = QJsonDocument::fromJson(errData);
            const QString msg = errDoc.object().value("message").toString(reply->errorString());
            emit friendActionFailed(msg);
            return;
        }

        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonObject mObj = doc.object().value("message").toObject();

        ChatMessageModel msg;
        msg.id = mObj.value("id").toString();
        msg.fromUserId = mObj.value("fromUserId").toString();
        msg.fromUsername = mObj.value("fromUsername").toString();
        msg.toUserId = mObj.value("toUserId").toString();
        msg.toUsername = mObj.value("toUsername").toString();
        msg.text = mObj.value("text").toString();
        msg.timestamp = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(mObj.value("timestamp").toDouble()));
        msg.isRead = true;

        emit chatMessageSent(msg);
    });
}

void SVLFriendManager::fetchChatHistory(const QString& targetUsername) {
    if (!validateUsername(targetUsername)) return;

    QUrl url(m_masterApiUrl + "/api/v1/friends/chat");
    QUrlQuery query;
    query.addQueryItem("with", targetUsername.trimmed());
    url.setQuery(query);

    QNetworkRequest request(url);
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    auto* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, targetUsername]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }

        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonArray arr = doc.object().value("messages").toArray();

        QList<ChatMessageModel> messages;
        for (const auto& val : arr) {
            const QJsonObject mObj = val.toObject();
            ChatMessageModel msg;
            msg.id = mObj.value("id").toString();
            msg.fromUserId = mObj.value("fromUserId").toString();
            msg.fromUsername = mObj.value("fromUsername").toString();
            msg.toUserId = mObj.value("toUserId").toString();
            msg.toUsername = mObj.value("toUsername").toString();
            msg.text = mObj.value("text").toString();
            msg.timestamp = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(mObj.value("timestamp").toDouble()));
            msg.isRead = mObj.value("read").toBool();
            messages.append(msg);
        }

        emit chatHistoryLoaded(targetUsername, messages);
    });
}

void SVLFriendManager::sendGameInvite(const QString& targetUsername, const QString& serverName, const QString& address, quint16 port) {
    if (!validateUsername(targetUsername)) {
        emit friendActionFailed(tr("Invalid recipient username."));
        return;
    }
    if (!validateAddress(address)) {
        emit friendActionFailed(tr("Invalid server address for game invite."));
        return;
    }

    QUrl url(m_masterApiUrl + "/api/v1/friends/invite");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject payload;
    payload["targetUsername"] = targetUsername.trimmed();
    payload["serverName"] = serverName.trimmed();
    payload["address"] = address.trimmed();
    payload["port"] = port;

    auto* reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, targetUsername]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const QByteArray errData = reply->readAll();
            const QJsonDocument errDoc = QJsonDocument::fromJson(errData);
            const QString msg = errDoc.object().value("message").toString(reply->errorString());
            emit friendActionFailed(msg);
            return;
        }

        emit friendActionSucceeded(tr("Game invite sent to %1!").arg(targetUsername));
    });
}

void SVLFriendManager::respondGameInvite(const QString& inviteId, bool accept) {
    if (inviteId.isEmpty()) return;

    QUrl url(m_masterApiUrl + "/api/v1/friends/invite/respond");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QJsonObject payload;
    payload["inviteId"] = inviteId;
    payload["accept"] = accept;

    auto* reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, accept]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError && accept) {
            const QByteArray data = reply->readAll();
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            const QJsonObject invObj = doc.object().value("invite").toObject();
            const QString addr = invObj.value("targetAddress").toString();
            const quint16 port = static_cast<quint16>(invObj.value("targetPort").toInt(25565));

            if (validateAddress(addr)) {
                emit joinFriendRequested(addr, port);
            }
        }
        pollStatus();
    });
}

void SVLFriendManager::updateMyPresence(FriendActivityState state, const QString& details, const QString& address, quint16 port) {
    QUrl url(m_masterApiUrl + "/api/v1/friends/status");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    SVLSecurity::injectAuthHeaders(request);
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_authToken.toUtf8());
    }

    QString stateStr = "Online";
    if (state == FriendActivityState::InSingleplayerP2P) stateStr = "InSingleplayerP2P";
    else if (state == FriendActivityState::OnCloudServer) stateStr = "OnCloudServer";
    else if (state == FriendActivityState::OnSunveilSMP) stateStr = "OnSunveilSMP";
    else if (state == FriendActivityState::Offline) stateStr = "Offline";

    QJsonObject payload;
    payload["state"] = stateStr;
    payload["details"] = details;
    if (!address.isEmpty() && validateAddress(address)) {
        payload["address"] = address;
        payload["port"] = port;
    }
    if (!m_myUsername.isEmpty()) {
        payload["minecraftUsername"] = m_myUsername;
    }

    auto* reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        reply->deleteLater();
    });
}
