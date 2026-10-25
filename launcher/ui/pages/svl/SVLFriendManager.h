#pragma once

#include <QObject>
#include <QList>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "SVLFriendModel.h"

class SVLFriendManager : public QObject {
    Q_OBJECT

public:
    explicit SVLFriendManager(QNetworkAccessManager* nam, QObject* parent = nullptr);
    ~SVLFriendManager() override = default;

    void setAuthToken(const QString& token) { m_authToken = token; }
    void setMyUsername(const QString& username) { m_myUsername = username; }
    const QString& myUsername() const { return m_myUsername; }

    void startPolling(int intervalMs = 8000);
    void stopPolling();

    // Friend Management & Handshake
    void addFriend(const QString& username);
    void acceptFriend(const QString& username);
    void declineFriend(const QString& username);
    void removeFriend(const QString& username);
    void blockUser(const QString& username);
    void unblockUser(const QString& username);
    void refreshFriends();

    // Direct 1-on-1 Chat
    void sendChatMessage(const QString& targetUsername, const QString& text);
    void fetchChatHistory(const QString& targetUsername);

    // Game Invites
    void sendGameInvite(const QString& targetUsername, const QString& serverName, const QString& address, quint16 port);
    void respondGameInvite(const QString& inviteId, bool accept);

    // Presence
    void updateMyPresence(FriendActivityState state, const QString& details, const QString& address = QString(), quint16 port = 25565);

    const QList<FriendStatusModel>& friends() const { return m_friends; }
    const QList<FriendRequestModel>& incomingRequests() const { return m_incomingRequests; }
    const QList<FriendRequestModel>& outgoingRequests() const { return m_outgoingRequests; }
    const QList<GameInviteModel>& pendingInvites() const { return m_pendingInvites; }

signals:
    void friendsUpdated(const QList<FriendStatusModel>& friends);
    void requestsUpdated(const QList<FriendRequestModel>& incoming, const QList<FriendRequestModel>& outgoing);
    void gameInviteReceived(const GameInviteModel& invite);
    void chatHistoryLoaded(const QString& withUser, const QList<ChatMessageModel>& messages);
    void chatMessageSent(const ChatMessageModel& message);
    void joinFriendRequested(const QString& address, quint16 port);
    void friendActionSucceeded(const QString& message);
    void friendActionFailed(const QString& error);

private slots:
    void pollStatus();
    void onFriendsReplyFinished();

private:
    void postSimpleAction(const QString& endpoint, const QString& targetUsername, const QString& successMsg = QString());
    bool validateUsername(const QString& username) const;
    bool validateAddress(const QString& address) const;

    QNetworkAccessManager* m_nam = nullptr;
    QTimer m_pollTimer;
    QString m_authToken;
    QString m_myUsername;
    QString m_masterApiUrl = "https://realms.sunveil.net";

    QList<FriendStatusModel> m_friends;
    QList<FriendRequestModel> m_incomingRequests;
    QList<FriendRequestModel> m_outgoingRequests;
    QList<GameInviteModel> m_pendingInvites;
    QSet<QString> m_notifiedInviteIds;

    QNetworkReply* m_pollReply = nullptr;
};
