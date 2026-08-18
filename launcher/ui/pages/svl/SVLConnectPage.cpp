#include "SVLConnectPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include <QTimer>

#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QIcon>
#include "Application.h"
#include "SVLSecurity.h"
#include "tasks/SVLModSyncTask.h"
#include "ui/dialogs/SVLLoadingOverlay.h"
#include "ui/dialogs/SVLErrorOverlay.h"
#include "minecraft/MinecraftInstance.h"

SVLConnectPage::SVLConnectPage(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    refreshServers();
}

SVLConnectPage::~SVLConnectPage()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

QPixmap SVLConnectPage::createRoundedIcon(const QPixmap& src, int width, int height, int radius)
{
    if (src.isNull()) {
        return QPixmap();
    }

    QPixmap scaled = src.scaled(width, height, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap dest(width, height);
    dest.fill(Qt::transparent);

    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addRoundedRect(0, 0, width, height, radius, radius);
    painter.setClipPath(path);

    int x = (width - scaled.width()) / 2;
    int y = (height - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    return dest;
}

QPixmap SVLConnectPage::loadServerIcon(const QString& iconData, int width, int height, int radius)
{
    QPixmap pixmap;

    if (!iconData.isEmpty()) {
        if (iconData.startsWith("data:image/") || iconData.contains(";base64,")) {
            int commaIdx = iconData.indexOf(",");
            QString b64 = (commaIdx != -1) ? iconData.mid(commaIdx + 1) : iconData;
            QByteArray decoded = QByteArray::fromBase64(b64.toLatin1());
            pixmap.loadFromData(decoded);
        } else if (iconData.startsWith(":/") || iconData.startsWith("qrc:/")) {
            pixmap.load(iconData);
        } else {
            QByteArray decoded = QByteArray::fromBase64(iconData.toLatin1());
            if (!decoded.isEmpty()) {
                pixmap.loadFromData(decoded);
            }
            if (pixmap.isNull()) {
                pixmap.load(iconData);
            }
        }
    }

    // Fallback: Default generic cube or Sunveil logo SVG
    if (pixmap.isNull()) {
        QIcon fallbackIcon = QIcon::fromTheme("server", QIcon(":/icons/pe_dark/scalable/server.svg"));
        pixmap = fallbackIcon.pixmap(width, height);
    }

    return createRoundedIcon(pixmap, width, height, radius);
}

void SVLConnectPage::bindServerIcon(const QString& iconData, QLabel* label, int size)
{
    if (!label) {
        return;
    }

    if (iconData.startsWith("http://") || iconData.startsWith("https://")) {
        if (m_iconCache.contains(iconData)) {
            label->setPixmap(m_iconCache.value(iconData));
            return;
        }

        // Set fallback icon while network request is in flight
        label->setPixmap(loadServerIcon(QString(), size, size, 8));

        QUrl url(iconData);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
        req.setTransferTimeout(5000);

        QPointer<QLabel> targetLabel(label);
        auto* reply = APPLICATION->network()->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, targetLabel, iconData, size]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray bytes = reply->readAll();
                QPixmap raw;
                if (raw.loadFromData(bytes)) {
                    QPixmap rounded = createRoundedIcon(raw, size, size, 8);
                    m_iconCache.insert(iconData, rounded);
                    if (targetLabel) {
                        targetLabel->setPixmap(rounded);
                    }
                }
            }
        });
    } else {
        label->setPixmap(loadServerIcon(iconData, size, size, 8));
    }
}

void SVLConnectPage::setupUI()
{
    setObjectName("SVLConnectPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#SVLConnectPage { background-color: #111111; }");

    // Centered layout with max width 1200px constraint using horizontal expanding spacers
    auto* outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    outerLayout->addStretch(1);

    auto* centralContainer = new QWidget(this);
    centralContainer->setMaximumWidth(1200);
    centralContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    centralContainer->setStyleSheet("background-color: transparent; border: none;");

    auto* mainLayout = new QVBoxLayout(centralContainer);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    // 1. Top Header Row
    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(12);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(3);

    auto* titleLabel = new QLabel(tr("Realms"), centralContainer);
    titleLabel->setObjectName("pageTitleLabel");
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 20px; font-weight: 900; letter-spacing: 0.2px; background: transparent; border: none;");
    titleLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(tr("Select a server to automatically synchronize mods and connect."), centralContainer);
    subtitleLabel->setObjectName("pageSubtitleLabel");
    subtitleLabel->setStyleSheet("color: #A1A1AA; font-size: 13px; font-weight: 400; background: transparent; border: none;");
    titleLayout->addWidget(subtitleLabel);

    headerRow->addLayout(titleLayout, 1);

    m_statusLabel = new QLabel(tr("1 SERVER AVAILABLE"), centralContainer);
    m_statusLabel->setObjectName("playerCountBadge");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("background-color: #111111; color: #00E599; border: 1px solid #2C2C2E; border-radius: 6px; padding: 5px 12px; font-size: 11px; font-weight: 700;");
    headerRow->addWidget(m_statusLabel, 0, Qt::AlignRight | Qt::AlignVCenter);

    mainLayout->addLayout(headerRow);

    // 2. Search & Controls Bar
    auto* searchRow = new QHBoxLayout();
    searchRow->setSpacing(10);

    m_searchEdit = new QLineEdit(centralContainer);
    m_searchEdit->setObjectName("realmSearchInput");
    m_searchEdit->setPlaceholderText(tr("Search realms by name, version or modloader..."));
    m_searchEdit->setStyleSheet("QLineEdit { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 8px; padding: 10px 16px; color: #FFFFFF; font-size: 13px; } QLineEdit:hover { border-color: #3F3F46; } QLineEdit:focus { border: 1px solid #00E599; }");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SVLConnectPage::onSearchFilterChanged);
    searchRow->addWidget(m_searchEdit, 1);

    m_refreshBtn = new QPushButton(tr("REFRESH"), centralContainer);
    m_refreshBtn->setObjectName("refreshButton");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 8px; padding: 10px 20px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #3F3F46; border-color: #52525B; } QPushButton:pressed { background-color: #1C1C1E; } QPushButton:disabled { background-color: #1C1C1E; color: #71717A; }");
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        // 1. Immediately disable the button to prevent spam
        m_refreshBtn->setEnabled(false);
        m_refreshBtn->setText(tr("REFRESHING..."));

        // 2. Trigger the actual API fetch logic
        this->refreshServers();

        // 3. Re-enable the button after a 2.5 second cooldown
        QTimer::singleShot(2500, this, [this]() {
            if (m_refreshBtn) {
                m_refreshBtn->setEnabled(true);
                m_refreshBtn->setText(tr("REFRESH"));
            }
        });
    });
    searchRow->addWidget(m_refreshBtn);

    mainLayout->addLayout(searchRow);

    // 3. Scroll Area for Server Cards
    m_scrollArea = new QScrollArea(centralContainer);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("background: transparent; border: none;");
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* scrollContainer = new QWidget();
    scrollContainer->setStyleSheet("background-color: transparent; border: none;");
    m_cardsLayout = new QVBoxLayout(scrollContainer);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(14);
    m_cardsLayout->addStretch();

    m_scrollArea->setWidget(scrollContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    outerLayout->addWidget(centralContainer, 10);
    outerLayout->addStretch(1);
}

void SVLConnectPage::refreshServers()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_statusLabel) {
        m_statusLabel->setText(tr("Connecting..."));
    }

    QUrl url(m_masterApiBaseUrl + "/api/v1/servers");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(4000);
    SVLSecurity::injectAuthHeaders(request);

    m_currentReply = APPLICATION->network()->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &SVLConnectPage::onServersReceived);
}

void SVLConnectPage::onServersReceived()
{
    if (!m_currentReply) {
        return;
    }

    auto reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    m_allServers.clear();
    bool hasApiError = (reply->error() != QNetworkReply::NoError);

    if (!hasApiError) {
        QByteArray responsePayload = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responsePayload);
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const auto& val : array) {
                if (val.isObject()) {
                    QJsonObject obj = val.toObject();
                    SVLServerModel model;
                    model.serverKey = obj.value("serverKey").toString();
                    model.name = obj.value("name").toString(model.serverKey);
                    model.icon = obj.value("icon").toString(obj.value("logo").toString());
                    model.ip = obj.value("ip").toString("127.0.0.1");
                    model.port = static_cast<quint16>(obj.value("port").toInt(25565));

                    QJsonObject verObj = obj.value("version").toObject();
                    model.mcVersion = verObj.value("minecraft").toString(obj.value("mcVersion").toString("1.21.1"));
                    model.loader = verObj.value("loader").toString(obj.value("loader").toString("forge"));
                    model.loaderVersion = verObj.value("loaderVersion").toString(obj.value("loaderVersion").toString("52.0.18"));

                    QJsonObject statObj = obj.value("status").toObject();
                    model.players = statObj.value("players").toInt(obj.value("players").toInt(0));
                    model.maxPlayers = statObj.value("maxPlayers").toInt(obj.value("maxPlayers").toInt(20));
                    model.motd = statObj.value("motd").toString(obj.value("motd").toString());
                    model.verified = obj.value("verified").toBool(true);

                    if (model.icon.isEmpty() && statObj.contains("favicon")) {
                        model.icon = statObj.value("favicon").toString();
                    }

                    if (obj.contains("mods") && obj.value("mods").isArray()) {
                        model.modCount = obj.value("mods").toArray().size();
                    } else {
                        model.modCount = obj.value("modCount").toInt(13);
                    }
                    model.isOnline = obj.contains("online") ? obj.value("online").toBool(true) : (statObj.contains("online") ? statObj.value("online").toBool(true) : true);

                    model.boosts = obj.value("boosts").toInt(0);
                    model.sponsored = obj.value("sponsored").toBool(false);
                    model.bannerUrl = obj.value("bannerUrl").toString();

                    QJsonObject linksObj = obj.value("links").toObject();
                    model.links.store = linksObj.value("store").toString();
                    model.links.discord = linksObj.value("discord").toString();
                    model.links.website = linksObj.value("website").toString();

                    m_allServers.append(model);
                }
            }
        }
    }

    // Supply official default server if offline or empty
    if (m_allServers.isEmpty()) {
        SVLServerModel defaultServer;
        defaultServer.serverKey = "sunveil-modded";
        defaultServer.name = "Sunveil Modded Server";
        defaultServer.icon = ""; // Null -> triggers fallback generic cube / Sunveil logo
        defaultServer.ip = "play.sunveil.net";
        defaultServer.port = 25565;
        defaultServer.mcVersion = "1.21.1";
        defaultServer.loader = "forge";
        defaultServer.loaderVersion = "52.0.18";
        defaultServer.players = 0;
        defaultServer.maxPlayers = 20;
        defaultServer.motd = "Official High-Performance Modded Survival & Adventure Infrastructure.";
        defaultServer.verified = true;
        defaultServer.modCount = 13;
        defaultServer.isOnline = !hasApiError;
        m_allServers.append(defaultServer);
    }

    onSearchFilterChanged(m_currentQuery);
}

void SVLConnectPage::onSearchFilterChanged(const QString& query)
{
    m_currentQuery = query.trimmed().toLower();
    m_filteredServers.clear();

    for (const auto& srv : m_allServers) {
        if (m_currentQuery.isEmpty() ||
            srv.name.toLower().contains(m_currentQuery) ||
            srv.serverKey.toLower().contains(m_currentQuery) ||
            srv.motd.toLower().contains(m_currentQuery) ||
            srv.mcVersion.toLower().contains(m_currentQuery) ||
            srv.loader.toLower().contains(m_currentQuery)) {
            m_filteredServers.append(srv);
        }
    }

    if (m_statusLabel) {
        m_statusLabel->setText(tr("%1 %2 AVAILABLE").arg(m_filteredServers.size()).arg(m_filteredServers.size() == 1 ? tr("SERVER") : tr("SERVERS")));
    }

    renderServerCards();
}

void SVLConnectPage::renderServerCards()
{
    if (!m_cardsLayout) {
        return;
    }

    // Clear previous cards
    QLayoutItem* item;
    while ((item = m_cardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (m_filteredServers.isEmpty()) {
        auto* emptyLabel = new QLabel(tr("No active Sunveil realms online. Click 'Refresh' to check again."), this);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #71717A; font-size: 13px; padding: 40px;");
        m_cardsLayout->addWidget(emptyLabel);
        m_cardsLayout->addStretch();
        return;
    }

    for (int i = 0; i < m_filteredServers.size(); ++i) {
        m_cardsLayout->addWidget(createServerCard(m_filteredServers[i]));
    }

    m_cardsLayout->addStretch();
}

QWidget* SVLConnectPage::createServerCard(const SVLServerModel& server)
{
    auto* card = new QFrame(this);
    card->setObjectName("serverCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    bool isBoostedOrSponsored = (server.boosts > 0 || server.sponsored);
    if (isBoostedOrSponsored) {
        card->setStyleSheet("QFrame#serverCard { border: 1px solid #FFB800; background-color: #1A1A12; border-radius: 12px; } QFrame#serverCard:hover { border: 1px solid #FFC72C; background-color: #24241A; }");
    } else {
        card->setStyleSheet("QFrame#serverCard { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 12px; } QFrame#serverCard:hover { background-color: #242426; border: 1px solid #3F3F46; }");
    }

    auto* cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(18);

    // Left: 56x56 Server Icon (Smooth transformed with 8px border radius)
    auto* iconContainer = new QFrame(card);
    iconContainer->setFixedSize(56, 56);
    iconContainer->setStyleSheet("background-color: #111111; border: 1px solid #2C2C2E; border-radius: 8px;");
    auto* iconInnerLayout = new QVBoxLayout(iconContainer);
    iconInnerLayout->setContentsMargins(0, 0, 0, 0);
    iconInnerLayout->setAlignment(Qt::AlignCenter);

    auto* avatarIcon = new QLabel(iconContainer);
    avatarIcon->setAlignment(Qt::AlignCenter);
    avatarIcon->setStyleSheet("background: transparent; border: none;");
    bindServerIcon(server.icon, avatarIcon, 56);
    iconInnerLayout->addWidget(avatarIcon);
    cardLayout->addWidget(iconContainer, 0, Qt::AlignVCenter);

    // Center: Name, MOTD, Badges
    auto* centerLayout = new QVBoxLayout();
    centerLayout->setSpacing(5);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(10);

    auto* nameLabel = new QLabel(server.name, card);
    nameLabel->setObjectName("serverTitle");
    nameLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 700; background: transparent; border: none;");
    titleRow->addWidget(nameLabel);

    if (server.boosts > 0) {
        auto* boostedBadge = new QLabel(QString("🔥 BOOSTED (%1)").arg(server.boosts), card);
        boostedBadge->setObjectName("badgeBoosted");
        boostedBadge->setStyleSheet("background-color: rgba(255, 184, 0, 0.15); color: #FFB800; border: 1px solid rgba(255, 184, 0, 0.4); border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 800;");
        titleRow->addWidget(boostedBadge);
    } else if (server.sponsored) {
        auto* sponsoredBadge = new QLabel(tr("★ SPONSORED"), card);
        sponsoredBadge->setObjectName("badgeBoosted");
        sponsoredBadge->setStyleSheet("background-color: rgba(255, 184, 0, 0.15); color: #FFB800; border: 1px solid rgba(255, 184, 0, 0.4); border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 800;");
        titleRow->addWidget(sponsoredBadge);
    }

    if (server.verified) {
        auto* verifiedBadge = new QLabel(tr("VERIFIED REALM"), card);
        verifiedBadge->setObjectName("badgeVerified");
        verifiedBadge->setStyleSheet("background-color: rgba(0, 229, 153, 0.12); color: #00E599; border: 1px solid rgba(0, 229, 153, 0.35); border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 700;");
        titleRow->addWidget(verifiedBadge);
    }

    titleRow->addStretch();
    centerLayout->addLayout(titleRow);

    auto* motdLabel = new QLabel(server.motd.isEmpty() ? tr("Official High-Performance Modded Survival & Adventure Infrastructure.") : server.motd, card);
    motdLabel->setObjectName("serverMotd");
    motdLabel->setWordWrap(true);
    motdLabel->setStyleSheet("color: #A1A1AA; font-size: 13px; font-weight: 400; background: transparent; border: none;");
    centerLayout->addWidget(motdLabel);

    // Horizontal Badges Row
    auto* badgesLayout = new QHBoxLayout();
    badgesLayout->setSpacing(8);

    auto* onlineBadge = new QLabel(card);
    if (server.isOnline) {
        onlineBadge->setText(tr("● ONLINE"));
        onlineBadge->setStyleSheet("color: #00E599; font-size: 11px; font-weight: 700; background: transparent; border: none; padding-right: 4px;");
    } else {
        onlineBadge->setText(tr("● OFFLINE"));
        onlineBadge->setStyleSheet("color: #EF4444; font-size: 11px; font-weight: 700; background: transparent; border: none; padding-right: 4px;");
    }
    badgesLayout->addWidget(onlineBadge);

    auto* verLabel = new QLabel(QString("⚡ %1 %2").arg(server.loader.toUpper(), server.mcVersion), card);
    verLabel->setObjectName("badgeMeta");
    verLabel->setStyleSheet("background-color: #111111; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 6px; padding: 3px 8px; font-size: 11px; font-weight: 600;");
    badgesLayout->addWidget(verLabel);

    if (server.modCount > 0) {
        auto* modsLabel = new QLabel(tr("📦 %1 Mods").arg(server.modCount), card);
        modsLabel->setObjectName("badgeMeta");
        modsLabel->setStyleSheet("background-color: #111111; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 6px; padding: 3px 8px; font-size: 11px; font-weight: 600;");
        badgesLayout->addWidget(modsLabel);
    }

    if (server.isOnline) {
        auto* pingLabel = new QLabel(tr("📶 24ms EU"), card);
        pingLabel->setObjectName("badgeMeta");
        pingLabel->setStyleSheet("background-color: #111111; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 6px; padding: 3px 8px; font-size: 11px; font-weight: 600;");
        badgesLayout->addWidget(pingLabel);
    }

    badgesLayout->addStretch();
    centerLayout->addLayout(badgesLayout);

    cardLayout->addLayout(centerLayout, 1);

    // Right: Player Count + Action Buttons
    auto* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* playersLabel = new QLabel(QString("👥 %1 / %2 PLAYERS").arg(server.players).arg(server.maxPlayers), card);
    playersLabel->setObjectName("playerCountBadge");
    playersLabel->setAlignment(Qt::AlignCenter);
    playersLabel->setStyleSheet("background-color: #111111; color: #00E599; border: 1px solid #2C2C2E; border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    playersLabel->setVisible(server.isOnline); // Hide player count if offline
    rightLayout->addWidget(playersLabel, 0, Qt::AlignRight);

    auto* actionButtonsLayout = new QHBoxLayout();
    actionButtonsLayout->setSpacing(8);

    auto* detailsBtn = new QPushButton(tr("Details"), card);
    detailsBtn->setObjectName("cardDetailsBtn");
    detailsBtn->setCursor(Qt::PointingHandCursor);
    detailsBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 8px; padding: 8px 16px; font-weight: 600; font-size: 12px; min-height: 20px; } QPushButton:hover { background-color: #3F3F46; border-color: #52525B; } QPushButton:pressed { background-color: #1C1C1E; } QPushButton:disabled { background-color: #1C1C1E; color: #71717A; }");
    connect(detailsBtn, &QPushButton::clicked, this, [this, server, detailsBtn]() {
        detailsBtn->setEnabled(false);
        emit serverDetailsRequested(server);
        QTimer::singleShot(800, detailsBtn, [detailsBtn]() {
            if (detailsBtn) {
                detailsBtn->setEnabled(true);
            }
        });
    });
    actionButtonsLayout->addWidget(detailsBtn);

    auto* connectBtn = new QPushButton(card);
    connectBtn->setObjectName("btnConnect");
    if (server.isOnline) {
        connectBtn->setText(tr("CONNECT"));
        connectBtn->setEnabled(true);
        connectBtn->setCursor(Qt::PointingHandCursor);
        connectBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 13px; font-weight: 900; letter-spacing: 0.5px; border: none; border-radius: 8px; padding: 8px 24px; min-height: 20px; } QPushButton:hover { background-color: #10FFAC; color: #000000; } QPushButton:pressed { background-color: #00B377; } QPushButton:disabled { background-color: #2C2C2E; color: #52525B; }");
        connect(connectBtn, &QPushButton::clicked, this, [this, server, connectBtn]() {
            connectBtn->setEnabled(false);
            connectBtn->setText(tr("CONNECTING..."));
            onConnectClicked(server);
            if (connectBtn) {
                connectBtn->setEnabled(true);
                connectBtn->setText(tr("CONNECT"));
            }
        });
    } else {
        connectBtn->setText(tr("OFFLINE"));
        connectBtn->setEnabled(false);
        connectBtn->setCursor(Qt::ForbiddenCursor);
        connectBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #71717A; font-size: 13px; font-weight: 800; border: 1px solid #3F3F46; border-radius: 8px; padding: 8px 24px; min-height: 20px; }");
    }
    actionButtonsLayout->addWidget(connectBtn);

    rightLayout->addLayout(actionButtonsLayout);
    cardLayout->addLayout(rightLayout, 0);

    return card;
}

void SVLConnectPage::launchServer(const SVLServerModel& server)
{
    onConnectClicked(server);
}

void SVLConnectPage::onConnectClicked(const SVLServerModel& server)
{
    auto* syncTask = new SVLModSyncTask(m_masterApiBaseUrl, server.serverKey, server.name, server.ip, server.port, this);

    auto* overlay = new SVLLoadingOverlay(this->window());
    overlay->setPrimaryStatus(tr("CONNECTING TO %1").arg(server.name.toUpper()));
    overlay->setDetailStatus(tr("Verifying realm manifest and synchronizing client assets..."));

    connect(syncTask, &SVLModSyncTask::status, overlay, [overlay](const QString& statusText) {
        overlay->setPrimaryStatus(statusText);
    });

    connect(syncTask, &SVLModSyncTask::progress, overlay, [overlay](qint64 current, qint64 total) {
        overlay->setProgress(static_cast<int>(current), static_cast<int>(total));
    });

    connect(syncTask, &SVLModSyncTask::readyToLaunch, this, [this](MinecraftInstance* inst, const QString& ip, quint16 port) {
        emit launchRequested(inst, ip, port);
    });

    connect(syncTask, &Task::succeeded, overlay, [overlay]() {
        overlay->accept();
    });

    connect(syncTask, &Task::failed, overlay, [this, overlay, server](const QString& reason) {
        overlay->reject();
        bool retry = SVLErrorOverlay::showError(
            this,
            tr("CONNECTION FAILED"),
            tr("Sunveil Master API unreachable"),
            tr("The server manifest could not be synchronized. The network might be temporarily offline or unreachable."),
            reason,
            true
        );
        if (retry) {
            onConnectClicked(server);
        }
    });

    connect(overlay, &SVLLoadingOverlay::cancelRequested, syncTask, [syncTask]() {
        syncTask->abort();
    });

    syncTask->start();
    overlay->exec();
    overlay->deleteLater();
}
