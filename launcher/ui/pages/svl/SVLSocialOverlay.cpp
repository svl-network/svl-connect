#include "SVLSocialOverlay.h"
#include "SVLFriendManager.h"
#include "Application.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QInputDialog>
#include <QRegularExpression>
#include <QScrollBar>

SVLSocialOverlay::SVLSocialOverlay(SVLFriendManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    setupUI();

    if (m_manager) {
        connect(m_manager, &SVLFriendManager::friendsUpdated, this, &SVLSocialOverlay::renderFriendList);
        connect(m_manager, &SVLFriendManager::requestsUpdated, this, &SVLSocialOverlay::renderRequests);
        connect(m_manager, &SVLFriendManager::gameInviteReceived, this, &SVLSocialOverlay::handleGameInvite);
        connect(m_manager, &SVLFriendManager::chatHistoryLoaded, this, &SVLSocialOverlay::handleChatHistory);
        connect(m_manager, &SVLFriendManager::chatMessageSent, this, &SVLSocialOverlay::handleChatMessageSent);

        connect(m_manager, &SVLFriendManager::friendActionSucceeded, this, [this](const QString& msg) {
            // Non-blocking notification or subtle feedback
        });

        connect(m_manager, &SVLFriendManager::friendActionFailed, this, [this](const QString& err) {
            QMessageBox::warning(this, tr("Social Network"), tr("Action failed: %1").arg(err));
        });

        renderFriendList(m_manager->friends());
        renderRequests(m_manager->incomingRequests(), m_manager->outgoingRequests());
    }
}

void SVLSocialOverlay::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(14);

    // 1. Top Header
    auto* headerLayout = new QHBoxLayout();
    auto* titleLayout = new QVBoxLayout();
    auto* title = new QLabel(tr("Friends & Social Hub"), this);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #ffffff;");
    auto* subtitle = new QLabel(tr("Steam & Epic Games-style community: real-time presence, 1-on-1 direct chat, and one-click game invites."), this);
    subtitle->setStyleSheet("font-size: 13px; color: #94a3b8;");
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);

    m_onlineCountLabel = new QLabel(tr("0 Online"), this);
    m_onlineCountLabel->setStyleSheet("color: #10b981; font-weight: bold; font-size: 13px; background: #064e3b; padding: 6px 14px; border-radius: 6px;");

    m_refreshBtn = new QPushButton(tr("↻ Refresh"), this);
    m_refreshBtn->setStyleSheet("QPushButton { background: #262633; color: white; border-radius: 6px; padding: 6px 14px; font-weight: bold; } QPushButton:hover { background: #333344; }");
    connect(m_refreshBtn, &QPushButton::clicked, this, &SVLSocialOverlay::onRefreshClicked);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(m_onlineCountLabel);
    headerLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(headerLayout);

    // 2. Game Invite Toast Banner
    m_inviteBanner = new QWidget(this);
    m_inviteBanner->setObjectName("InviteBanner");
    m_inviteBanner->setStyleSheet(
        "#InviteBanner { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1e1b4b, stop:1 #312e81); border: 1px solid #6366f1; border-radius: 8px; padding: 10px; }"
    );
    auto* bannerLayout = new QHBoxLayout(m_inviteBanner);
    bannerLayout->setContentsMargins(14, 10, 14, 10);
    bannerLayout->setSpacing(12);

    m_inviteTextLabel = new QLabel(this);
    m_inviteTextLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");

    m_inviteAcceptBtn = new QPushButton(tr("🎮 Accept & Join Game"), this);
    m_inviteAcceptBtn->setStyleSheet("QPushButton { background: #10b981; color: white; font-weight: bold; padding: 8px 18px; border-radius: 6px; font-size: 13px; } QPushButton:hover { background: #059669; }");

    m_inviteDeclineBtn = new QPushButton(tr("✕ Decline"), this);
    m_inviteDeclineBtn->setStyleSheet("QPushButton { background: #374151; color: #d1d5db; font-weight: bold; padding: 8px 14px; border-radius: 6px; font-size: 13px; } QPushButton:hover { background: #4b5563; }");

    bannerLayout->addWidget(m_inviteTextLabel, 1);
    bannerLayout->addWidget(m_inviteAcceptBtn);
    bannerLayout->addWidget(m_inviteDeclineBtn);
    m_inviteBanner->setVisible(false);
    mainLayout->addWidget(m_inviteBanner);

    connect(m_inviteAcceptBtn, &QPushButton::clicked, this, [this]() {
        if (m_manager && !m_activeInviteId.isEmpty()) {
            m_manager->respondGameInvite(m_activeInviteId, true);
            m_inviteBanner->setVisible(false);
            m_activeInviteId.clear();
        }
    });

    connect(m_inviteDeclineBtn, &QPushButton::clicked, this, [this]() {
        if (m_manager && !m_activeInviteId.isEmpty()) {
            m_manager->respondGameInvite(m_activeInviteId, false);
            m_inviteBanner->setVisible(false);
            m_activeInviteId.clear();
        }
    });

    // 3. Navigation Tabs (Steam / Epic Games style)
    auto* tabsLayout = new QHBoxLayout();
    tabsLayout->setSpacing(10);

    auto makeTabBtn = [this](const QString& text) {
        auto* btn = new QPushButton(text, this);
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QPushButton { background: #181822; color: #94a3b8; font-weight: bold; font-size: 14px; border-radius: 6px; padding: 9px 18px; border: 1px solid #282836; }"
            "QPushButton:hover { background: #222230; color: #ffffff; }"
            "QPushButton:checked { background: #6366f1; color: #ffffff; border: 1px solid #4f46e5; }"
        );
        return btn;
    };

    m_friendsTabBtn = makeTabBtn(tr("👥 Friends"));
    m_requestsTabBtn = makeTabBtn(tr("📩 Requests"));
    m_chatTabBtn = makeTabBtn(tr("💬 Direct Chat"));

    m_friendsTabBtn->setChecked(true);

    tabsLayout->addWidget(m_friendsTabBtn);
    tabsLayout->addWidget(m_requestsTabBtn);
    tabsLayout->addWidget(m_chatTabBtn);
    tabsLayout->addStretch();
    mainLayout->addLayout(tabsLayout);

    connect(m_friendsTabBtn, &QPushButton::clicked, this, [this]() { onTabClicked(0); });
    connect(m_requestsTabBtn, &QPushButton::clicked, this, [this]() { onTabClicked(1); });
    connect(m_chatTabBtn, &QPushButton::clicked, this, [this]() { onTabClicked(2); });

    // 4. Tab Stack
    m_tabStack = new QStackedWidget(this);
    m_tabStack->addWidget(createFriendsTab());
    m_tabStack->addWidget(createRequestsTab());
    m_tabStack->addWidget(createChatTab());
    m_tabStack->setCurrentIndex(0);

    mainLayout->addWidget(m_tabStack, 1);
}

void SVLSocialOverlay::onTabClicked(int tabIndex) {
    m_friendsTabBtn->setChecked(tabIndex == 0);
    m_requestsTabBtn->setChecked(tabIndex == 1);
    m_chatTabBtn->setChecked(tabIndex == 2);
    m_tabStack->setCurrentIndex(tabIndex);
}

// -- TAB 0: FRIENDS LIST --

QWidget* SVLSocialOverlay::createFriendsTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(14);

    // Add friend input box
    auto* addBar = new QHBoxLayout();
    m_usernameInput = new QLineEdit(widget);
    m_usernameInput->setPlaceholderText(tr("Enter player's Minecraft username to send request..."));
    m_usernameInput->setStyleSheet(
        "QLineEdit { background: #1a1a24; border: 1px solid #313142; border-radius: 6px; padding: 10px 14px; color: #ffffff; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #6366f1; }"
    );
    connect(m_usernameInput, &QLineEdit::returnPressed, this, &SVLSocialOverlay::onAddClicked);

    m_addBtn = new QPushButton(tr("Add Friend"), widget);
    m_addBtn->setStyleSheet(
        "QPushButton { background: #6366f1; color: white; border-radius: 6px; padding: 10px 20px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background: #4f46e5; }"
    );
    connect(m_addBtn, &QPushButton::clicked, this, &SVLSocialOverlay::onAddClicked);

    addBar->addWidget(m_usernameInput);
    addBar->addWidget(m_addBtn);
    layout->addLayout(addBar);

    // Scroll Area for Friends List
    auto* scrollArea = new QScrollArea(widget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    auto* container = new QWidget();
    container->setStyleSheet("background: transparent;");
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(16);

    // Online Section
    auto* onlineHeader = new QLabel(tr("🟢 ONLINE FRIENDS"), container);
    onlineHeader->setStyleSheet("font-size: 12px; font-weight: bold; color: #10b981; letter-spacing: 1px;");
    containerLayout->addWidget(onlineHeader);

    auto* onlineWidget = new QWidget(container);
    m_onlineCardsLayout = new QVBoxLayout(onlineWidget);
    m_onlineCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_onlineCardsLayout->setSpacing(8);
    containerLayout->addWidget(onlineWidget);

    // Offline Section
    auto* offlineHeader = new QLabel(tr("⚫ OFFLINE FRIENDS"), container);
    offlineHeader->setStyleSheet("font-size: 12px; font-weight: bold; color: #6b7280; letter-spacing: 1px; margin-top: 10px;");
    containerLayout->addWidget(offlineHeader);

    auto* offlineWidget = new QWidget(container);
    m_offlineCardsLayout = new QVBoxLayout(offlineWidget);
    m_offlineCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_offlineCardsLayout->setSpacing(8);
    containerLayout->addWidget(offlineWidget);

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea, 1);

    return widget;
}

// -- TAB 1: REQUESTS --

QWidget* SVLSocialOverlay::createRequestsTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(14);

    auto* scrollArea = new QScrollArea(widget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    auto* container = new QWidget();
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(16);

    // Incoming Requests
    auto* inHeader = new QLabel(tr("📥 INCOMING FRIEND REQUESTS"), container);
    inHeader->setStyleSheet("font-size: 12px; font-weight: bold; color: #38bdf8; letter-spacing: 1px;");
    containerLayout->addWidget(inHeader);

    auto* inWidget = new QWidget(container);
    m_incomingCardsLayout = new QVBoxLayout(inWidget);
    m_incomingCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_incomingCardsLayout->setSpacing(8);
    containerLayout->addWidget(inWidget);

    // Outgoing Requests
    auto* outHeader = new QLabel(tr("📤 OUTGOING PENDING REQUESTS"), container);
    outHeader->setStyleSheet("font-size: 12px; font-weight: bold; color: #94a3b8; letter-spacing: 1px; margin-top: 10px;");
    containerLayout->addWidget(outHeader);

    auto* outWidget = new QWidget(container);
    m_outgoingCardsLayout = new QVBoxLayout(outWidget);
    m_outgoingCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_outgoingCardsLayout->setSpacing(8);
    containerLayout->addWidget(outWidget);

    m_noRequestsLabel = new QLabel(tr("No pending friend requests."), container);
    m_noRequestsLabel->setStyleSheet("color: #64748b; font-size: 13px; font-style: italic; margin-top: 10px;");
    containerLayout->addWidget(m_noRequestsLabel);

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    layout->addWidget(scrollArea, 1);

    return widget;
}

// -- TAB 2: DIRECT CHAT --

QWidget* SVLSocialOverlay::createChatTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(14);

    // Left sidebar: Friend Selector
    auto* leftBar = new QWidget(widget);
    leftBar->setFixedWidth(240);
    leftBar->setStyleSheet("background: #161622; border-radius: 8px; border: 1px solid #282836;");
    auto* leftLayout = new QVBoxLayout(leftBar);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(8);

    auto* chatListTitle = new QLabel(tr("💬 Conversations"), leftBar);
    chatListTitle->setStyleSheet("font-weight: bold; color: #ffffff; font-size: 14px;");
    leftLayout->addWidget(chatListTitle);

    m_chatFriendList = new QListWidget(leftBar);
    m_chatFriendList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { color: #ffffff; padding: 10px; border-radius: 6px; font-weight: 500; font-size: 13px; }"
        "QListWidget::item:hover { background: #222230; }"
        "QListWidget::item:selected { background: #6366f1; color: #ffffff; }"
    );
    leftLayout->addWidget(m_chatFriendList, 1);
    layout->addWidget(leftBar);

    connect(m_chatFriendList, &QListWidget::currentTextChanged, this, [this](const QString& text) {
        if (!text.isEmpty()) {
            openChatWith(text.trimmed());
        }
    });

    // Right panel: Message Thread & Input
    auto* rightPanel = new QWidget(widget);
    rightPanel->setStyleSheet("background: #14141d; border-radius: 8px; border: 1px solid #242432;");
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(16, 14, 16, 14);
    rightLayout->setSpacing(12);

    // Chat Header
    m_chatHeaderLabel = new QLabel(tr("Select a friend to start chatting"), rightPanel);
    m_chatHeaderLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #ffffff; border-bottom: 1px solid #242432; padding-bottom: 8px;");
    rightLayout->addWidget(m_chatHeaderLabel);

    // Message History ScrollArea
    m_chatScrollArea = new QScrollArea(rightPanel);
    m_chatScrollArea->setWidgetResizable(true);
    m_chatScrollArea->setFrameShape(QFrame::NoFrame);
    m_chatScrollArea->setStyleSheet("background: transparent;");

    m_chatMessagesWidget = new QWidget();
    m_chatMessagesWidget->setStyleSheet("background: transparent;");
    m_chatMessagesLayout = new QVBoxLayout(m_chatMessagesWidget);
    m_chatMessagesLayout->setContentsMargins(0, 0, 0, 0);
    m_chatMessagesLayout->setSpacing(10);
    m_chatMessagesLayout->addStretch();

    m_chatScrollArea->setWidget(m_chatMessagesWidget);
    rightLayout->addWidget(m_chatScrollArea, 1);

    // Input Bar
    auto* inputBar = new QHBoxLayout();
    m_chatInput = new QLineEdit(rightPanel);
    m_chatInput->setPlaceholderText(tr("Type a secure message... (Enter to send)"));
    m_chatInput->setStyleSheet(
        "QLineEdit { background: #1c1c28; border: 1px solid #313142; border-radius: 6px; padding: 10px 14px; color: #ffffff; font-size: 13px; }"
        "QLineEdit:focus { border: 1px solid #6366f1; }"
    );
    connect(m_chatInput, &QLineEdit::returnPressed, this, &SVLSocialOverlay::onSendChatMessage);

    m_chatSendBtn = new QPushButton(tr("Send ➤"), rightPanel);
    m_chatSendBtn->setStyleSheet(
        "QPushButton { background: #6366f1; color: white; border-radius: 6px; padding: 10px 20px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background: #4f46e5; }"
    );
    connect(m_chatSendBtn, &QPushButton::clicked, this, &SVLSocialOverlay::onSendChatMessage);

    inputBar->addWidget(m_chatInput, 1);
    inputBar->addWidget(m_chatSendBtn);
    rightLayout->addLayout(inputBar);

    layout->addWidget(rightPanel, 1);
    return widget;
}

void SVLSocialOverlay::onSendChatMessage() {
    if (m_currentChatPartner.isEmpty() || !m_manager) return;
    const QString text = m_chatInput->text().trimmed();
    if (text.isEmpty()) return;

    m_manager->sendChatMessage(m_currentChatPartner, text);
    m_chatInput->clear();
}

void SVLSocialOverlay::openChatWith(const QString& username) {
    m_currentChatPartner = username.trimmed();
    m_chatHeaderLabel->setText(tr("💬 Chatting with %1").arg(m_currentChatPartner));

    onTabClicked(2);

    // Clear old messages
    QLayoutItem* item;
    while ((item = m_chatMessagesLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_chatMessagesLayout->addStretch();

    // Fetch history
    if (m_manager) {
        m_manager->fetchChatHistory(m_currentChatPartner);
    }
}

void SVLSocialOverlay::handleChatHistory(const QString& withUser, const QList<ChatMessageModel>& messages) {
    if (withUser != m_currentChatPartner) return;

    // Clear layout
    QLayoutItem* item;
    while ((item = m_chatMessagesLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const auto& msg : messages) {
        appendChatMessageBubble(msg);
    }
    m_chatMessagesLayout->addStretch();

    // Scroll to bottom
    QScrollBar* bar = m_chatScrollArea->verticalScrollBar();
    if (bar) bar->setValue(bar->maximum());
}

void SVLSocialOverlay::handleChatMessageSent(const ChatMessageModel& msg) {
    if (msg.toUsername == m_currentChatPartner || msg.fromUsername == m_currentChatPartner) {
        appendChatMessageBubble(msg);
        QScrollBar* bar = m_chatScrollArea->verticalScrollBar();
        if (bar) bar->setValue(bar->maximum());
    }
}

void SVLSocialOverlay::appendChatMessageBubble(const ChatMessageModel& msg) {
    const bool isMe = (m_manager && msg.fromUsername == m_manager->myUsername());

    auto* bubbleContainer = new QWidget(m_chatMessagesWidget);
    auto* bLayout = new QHBoxLayout(bubbleContainer);
    bLayout->setContentsMargins(0, 2, 0, 2);

    auto* bubble = new QWidget(bubbleContainer);
    bubble->setObjectName("ChatBubble");
    bubble->setStyleSheet(
        isMe
            ? "#ChatBubble { background: #4f46e5; border-radius: 8px; padding: 8px 12px; }"
            : "#ChatBubble { background: #262636; border-radius: 8px; padding: 8px 12px; }"
    );

    auto* v = new QVBoxLayout(bubble);
    v->setContentsMargins(10, 8, 10, 8);
    v->setSpacing(4);

    auto* textLbl = new QLabel(bubble);
    textLbl->setTextFormat(Qt::PlainText); // HTML injection defense!
    textLbl->setText(msg.text);
    textLbl->setWordWrap(true);
    textLbl->setStyleSheet("color: #ffffff; font-size: 13px;");

    auto* timeLbl = new QLabel(bubble);
    timeLbl->setText(msg.timestamp.toString("HH:mm"));
    timeLbl->setStyleSheet("color: #cbd5e1; font-size: 10px; font-weight: 500;");

    v->addWidget(textLbl);
    v->addWidget(timeLbl);

    if (isMe) {
        bLayout->addStretch();
        bLayout->addWidget(bubble);
    } else {
        bLayout->addWidget(bubble);
        bLayout->addStretch();
    }

    m_chatMessagesLayout->insertWidget(m_chatMessagesLayout->count() - 1, bubbleContainer);
}

void SVLSocialOverlay::handleGameInvite(const GameInviteModel& invite) {
    showInviteBanner(invite);
}

void SVLSocialOverlay::showInviteBanner(const GameInviteModel& invite) {
    m_activeInviteId = invite.inviteId;
    m_inviteTextLabel->setText(tr("🎮 %1 has invited you to join '%2' (%3:%4)!")
        .arg(invite.fromUsername, invite.serverName, invite.targetAddress).arg(invite.targetPort));
    m_inviteBanner->setVisible(true);
}

void SVLSocialOverlay::onAddClicked() {
    const QString username = m_usernameInput->text().trimmed();
    if (!username.isEmpty() && m_manager) {
        m_manager->addFriend(username);
        m_usernameInput->clear();
    }
}

void SVLSocialOverlay::onRefreshClicked() {
    if (m_manager) {
        m_manager->refreshFriends();
    }
}

void SVLSocialOverlay::loadAvatar(const QString& username, QLabel* targetLabel, int size) {
    if (m_avatarCache.contains(username)) {
        targetLabel->setPixmap(m_avatarCache.value(username));
        return;
    }

    // SSRF & URL injection defense: Strict username regex
    static const QRegularExpression regex("^[a-zA-Z0-9_]{3,16}$");
    if (!regex.match(username).hasMatch()) return;

    const QUrl url(QString("https://mc-heads.net/avatar/%1/%2").arg(QString(QUrl::toPercentEncoding(username))).arg(size));
    QNetworkRequest req(url);
    auto* reply = APPLICATION->network()->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, username, targetLabel, size]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pix;
            pix.loadFromData(reply->readAll());
            if (!pix.isNull()) {
                QPixmap rounded(size, size);
                rounded.fill(Qt::transparent);
                QPainter painter(&rounded);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addRoundedRect(0, 0, size, size, 6, 6);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));

                m_avatarCache.insert(username, rounded);
                if (targetLabel) {
                    targetLabel->setPixmap(rounded);
                }
            }
        }
    });
}

void SVLSocialOverlay::renderFriendList(const QList<FriendStatusModel>& friends) {
    // Clear online and offline layouts
    QLayoutItem* item;
    while ((item = m_onlineCardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    while ((item = m_offlineCardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Update conversation sidebar list
    m_chatFriendList->clear();

    int onlineCount = 0;
    for (const auto& f : friends) {
        m_chatFriendList->addItem(f.username);

        if (f.state != FriendActivityState::Offline) {
            onlineCount++;
            m_onlineCardsLayout->addWidget(createFriendCard(f));
        } else {
            m_offlineCardsLayout->addWidget(createFriendCard(f));
        }
    }

    m_onlineCountLabel->setText(tr("%1 Online").arg(onlineCount));
}

void SVLSocialOverlay::renderRequests(const QList<FriendRequestModel>& incoming, const QList<FriendRequestModel>& outgoing) {
    QLayoutItem* item;
    while ((item = m_incomingCardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    while ((item = m_outgoingCardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (const auto& req : incoming) {
        m_incomingCardsLayout->addWidget(createRequestCard(req));
    }
    for (const auto& req : outgoing) {
        m_outgoingCardsLayout->addWidget(createRequestCard(req));
    }

    const bool hasAny = (!incoming.isEmpty() || !outgoing.isEmpty());
    m_noRequestsLabel->setVisible(!hasAny);
    m_requestsTabBtn->setText(incoming.isEmpty() ? tr("📩 Requests") : tr("📩 Requests (%1)").arg(incoming.size()));
}

QWidget* SVLSocialOverlay::createFriendCard(const FriendStatusModel& friendItem) {
    auto* card = new QWidget(this);
    card->setObjectName("FriendCard");
    card->setStyleSheet(
        "#FriendCard { background: #16161e; border: 1px solid #282836; border-radius: 8px; padding: 12px; }"
        "#FriendCard:hover { border: 1px solid #4338ca; }"
    );

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(14);

    // Avatar
    auto* avatar = new QLabel(card);
    avatar->setFixedSize(48, 48);
    avatar->setStyleSheet("background: #232330; border-radius: 6px;");
    loadAvatar(friendItem.username, avatar, 48);

    // Info Layout
    auto* infoLayout = new QVBoxLayout();
    auto* nameLabel = new QLabel(friendItem.username, card);
    nameLabel->setStyleSheet("font-weight: bold; color: #ffffff; font-size: 15px;");

    QString statusText;
    QString statusColor;
    bool canJoin = false;

    switch (friendItem.state) {
        case FriendActivityState::InSingleplayerP2P:
            statusText = tr("🎮 Hosting P2P World (%1)").arg(friendItem.activityDetails.isEmpty() ? "Tunnel Host" : friendItem.activityDetails);
            statusColor = "#38bdf8";
            canJoin = !friendItem.targetAddress.isEmpty();
            break;
        case FriendActivityState::OnCloudServer:
            statusText = tr("☁️ Playing Cloud Realm: %1").arg(friendItem.activityDetails);
            statusColor = "#c084fc";
            canJoin = !friendItem.targetAddress.isEmpty();
            break;
        case FriendActivityState::OnSunveilSMP:
            statusText = tr("⚡ Playing Sunveil SMP");
            statusColor = "#f59e0b";
            canJoin = true;
            break;
        case FriendActivityState::OnlineInLauncher:
            statusText = tr("🟢 Online (In Launcher)");
            statusColor = "#10b981";
            break;
        case FriendActivityState::Offline:
        default:
            statusText = tr("⚫ Offline");
            statusColor = "#6b7280";
            break;
    }

    auto* statusLabel = new QLabel(statusText, card);
    statusLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 500;").arg(statusColor));

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(statusLabel);

    layout->addWidget(avatar);
    layout->addLayout(infoLayout);
    layout->addStretch();

    // 1. Direct Chat button
    auto* chatBtn = new QPushButton(tr("💬 Chat"), card);
    chatBtn->setStyleSheet(
        "QPushButton { background: #312e81; color: #c7d2fe; font-weight: bold; border-radius: 6px; padding: 8px 14px; font-size: 12px; }"
        "QPushButton:hover { background: #3730a3; color: white; }"
    );
    connect(chatBtn, &QPushButton::clicked, this, [this, friendItem]() {
        openChatWith(friendItem.username);
    });
    layout->addWidget(chatBtn);

    // 2. Invite to game button
    auto* inviteBtn = new QPushButton(tr("✉ Invite"), card);
    inviteBtn->setStyleSheet(
        "QPushButton { background: #262633; color: #e2e8f0; font-weight: bold; border-radius: 6px; padding: 8px 14px; font-size: 12px; }"
        "QPushButton:hover { background: #37374a; }"
    );
    connect(inviteBtn, &QPushButton::clicked, this, [this, friendItem]() {
        bool ok = false;
        QString srv = QInputDialog::getText(this, tr("Invite Friend to Game"),
            tr("Enter server address to invite %1 to:").arg(friendItem.username), QLineEdit::Normal, "play.sunveil.net", &ok);
        if (ok && !srv.trimmed().isEmpty() && m_manager) {
            m_manager->sendGameInvite(friendItem.username, "Sunveil World", srv.trimmed(), 25565);
        }
    });
    layout->addWidget(inviteBtn);

    // 3. Join game button
    if (canJoin) {
        auto* joinBtn = new QPushButton(tr("🚀 Join Game"), card);
        joinBtn->setStyleSheet(
            "QPushButton { background: #10b981; color: white; font-weight: bold; border-radius: 6px; padding: 8px 16px; font-size: 12px; }"
            "QPushButton:hover { background: #059669; }"
        );
        connect(joinBtn, &QPushButton::clicked, this, [this, friendItem]() {
            emit joinRequested(friendItem.targetAddress, friendItem.targetPort);
        });
        layout->addWidget(joinBtn);
    }

    // 4. Remove button
    auto* removeBtn = new QPushButton(tr("✕"), card);
    removeBtn->setFixedSize(28, 28);
    removeBtn->setToolTip(tr("Remove friend"));
    removeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #ef4444; font-weight: bold; border-radius: 4px; font-size: 14px; }"
        "QPushButton:hover { background: #3f1d24; }"
    );
    connect(removeBtn, &QPushButton::clicked, this, [this, friendItem]() {
        if (QMessageBox::question(this, tr("Remove Friend"), tr("Remove %1 from your friends list?").arg(friendItem.username)) == QMessageBox::Yes) {
            if (m_manager) m_manager->removeFriend(friendItem.username);
        }
    });
    layout->addWidget(removeBtn);

    return card;
}

QWidget* SVLSocialOverlay::createRequestCard(const FriendRequestModel& req) {
    auto* card = new QWidget(this);
    card->setObjectName("RequestCard");
    card->setStyleSheet(
        "#RequestCard { background: #181824; border: 1px solid #282838; border-radius: 8px; padding: 10px; }"
    );

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(12);

    auto* avatar = new QLabel(card);
    avatar->setFixedSize(36, 36);
    avatar->setStyleSheet("background: #232330; border-radius: 6px;");
    loadAvatar(req.username, avatar, 36);

    auto* nameLabel = new QLabel(req.username, card);
    nameLabel->setStyleSheet("font-weight: bold; color: #ffffff; font-size: 14px;");

    layout->addWidget(avatar);
    layout->addWidget(nameLabel);
    layout->addStretch();

    if (req.isIncoming) {
        auto* acceptBtn = new QPushButton(tr("✔ Accept"), card);
        acceptBtn->setStyleSheet("QPushButton { background: #10b981; color: white; font-weight: bold; border-radius: 5px; padding: 6px 14px; } QPushButton:hover { background: #059669; }");
        connect(acceptBtn, &QPushButton::clicked, this, [this, req]() {
            if (m_manager) m_manager->acceptFriend(req.username);
        });

        auto* declineBtn = new QPushButton(tr("✕ Decline"), card);
        declineBtn->setStyleSheet("QPushButton { background: #374151; color: #e5e7eb; font-weight: bold; border-radius: 5px; padding: 6px 12px; } QPushButton:hover { background: #4b5563; }");
        connect(declineBtn, &QPushButton::clicked, this, [this, req]() {
            if (m_manager) m_manager->declineFriend(req.username);
        });

        layout->addWidget(acceptBtn);
        layout->addWidget(declineBtn);
    } else {
        auto* cancelBtn = new QPushButton(tr("Cancel Request"), card);
        cancelBtn->setStyleSheet("QPushButton { background: #262633; color: #94a3b8; font-weight: bold; border-radius: 5px; padding: 6px 12px; } QPushButton:hover { background: #333344; color: white; }");
        connect(cancelBtn, &QPushButton::clicked, this, [this, req]() {
            if (m_manager) m_manager->declineFriend(req.username);
        });
        layout->addWidget(cancelBtn);
    }

    return card;
}
