#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QHash>
#include <QPixmap>
#include "SVLFriendModel.h"

class SVLFriendManager;

class SVLSocialOverlay : public QWidget {
    Q_OBJECT

public:
    explicit SVLSocialOverlay(SVLFriendManager* manager, QWidget* parent = nullptr);
    ~SVLSocialOverlay() override = default;

public slots:
    void renderFriendList(const QList<FriendStatusModel>& friends);
    void renderRequests(const QList<FriendRequestModel>& incoming, const QList<FriendRequestModel>& outgoing);
    void handleGameInvite(const GameInviteModel& invite);
    void handleChatHistory(const QString& withUser, const QList<ChatMessageModel>& messages);
    void handleChatMessageSent(const ChatMessageModel& msg);
    void openChatWith(const QString& username);

signals:
    void joinRequested(const QString& address, quint16 port);

private slots:
    void onAddClicked();
    void onRefreshClicked();
    void onSendChatMessage();
    void onTabClicked(int tabIndex);

private:
    void setupUI();
    QWidget* createFriendsTab();
    QWidget* createRequestsTab();
    QWidget* createChatTab();
    QWidget* createFriendCard(const FriendStatusModel& friendItem);
    QWidget* createRequestCard(const FriendRequestModel& req);
    void loadAvatar(const QString& username, QLabel* targetLabel, int size = 48);
    void showInviteBanner(const GameInviteModel& invite);
    void appendChatMessageBubble(const ChatMessageModel& msg);

    SVLFriendManager* m_manager = nullptr;

    // Header & Navigation Tabs
    QPushButton* m_friendsTabBtn = nullptr;
    QPushButton* m_requestsTabBtn = nullptr;
    QPushButton* m_chatTabBtn = nullptr;
    QLabel* m_onlineCountLabel = nullptr;
    QPushButton* m_refreshBtn = nullptr;

    // Top Invite Banner
    QWidget* m_inviteBanner = nullptr;
    QLabel* m_inviteTextLabel = nullptr;
    QPushButton* m_inviteAcceptBtn = nullptr;
    QPushButton* m_inviteDeclineBtn = nullptr;
    QString m_activeInviteId;

    // Main Tab Stack
    QStackedWidget* m_tabStack = nullptr;

    // Tab 0: Friends List
    QLineEdit* m_usernameInput = nullptr;
    QPushButton* m_addBtn = nullptr;
    QVBoxLayout* m_onlineCardsLayout = nullptr;
    QVBoxLayout* m_offlineCardsLayout = nullptr;

    // Tab 1: Requests
    QVBoxLayout* m_incomingCardsLayout = nullptr;
    QVBoxLayout* m_outgoingCardsLayout = nullptr;
    QLabel* m_noRequestsLabel = nullptr;

    // Tab 2: Direct Chat
    QListWidget* m_chatFriendList = nullptr;
    QLabel* m_chatHeaderLabel = nullptr;
    QScrollArea* m_chatScrollArea = nullptr;
    QWidget* m_chatMessagesWidget = nullptr;
    QVBoxLayout* m_chatMessagesLayout = nullptr;
    QLineEdit* m_chatInput = nullptr;
    QPushButton* m_chatSendBtn = nullptr;
    QString m_currentChatPartner;

    QHash<QString, QPixmap> m_avatarCache;
};
