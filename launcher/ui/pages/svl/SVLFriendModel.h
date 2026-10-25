#pragma once

#include <QString>
#include <QDateTime>

enum class FriendActivityState {
    Offline,
    OnlineInLauncher,
    InSingleplayerP2P,
    OnCloudServer,
    OnSunveilSMP
};

struct FriendStatusModel {
    QString username;
    QString uuid;
    FriendActivityState state = FriendActivityState::Offline;
    QString activityDetails;  // e.g. "World: Dragon Lair" or "Server: Survival Cloud #2"
    QString targetAddress;    // Tunnel host / IP
    quint16 targetPort = 25565;
    QString avatarUrl;
    QDateTime lastSeen;
};

struct FriendRequestModel {
    QString userId;
    QString username;
    bool isIncoming = true;
};

struct ChatMessageModel {
    QString id;
    QString fromUserId;
    QString fromUsername;
    QString toUserId;
    QString toUsername;
    QString text;
    QDateTime timestamp;
    bool isRead = false;
};

struct GameInviteModel {
    QString inviteId;
    QString fromUserId;
    QString fromUsername;
    QString serverName;
    QString targetAddress;
    quint16 targetPort = 25565;
    QDateTime timestamp;
    QDateTime expiresAt;
};
