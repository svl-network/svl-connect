#include "SVLRealmDetailPage.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QTextBrowser>
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "Application.h"
#include "SVLSecurity.h"

SVLRealmDetailPage::SVLRealmDetailPage(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SVLRealmDetailPage::setupUI()
{
    setObjectName("SVLRealmDetailPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("QWidget#SVLRealmDetailPage { background-color: #111111; }");

    // Centered layout with max width 1200px
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

    // 1. Top navigation strip
    auto* topNavLayout = new QHBoxLayout();
    topNavLayout->setSpacing(12);

    m_backBtn = new QPushButton(tr("← BACK TO REALMS"), centralContainer);
    m_backBtn->setObjectName("detailBackButton");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #3F3F46; }");
    connect(m_backBtn, &QPushButton::clicked, this, &SVLRealmDetailPage::backRequested);
    topNavLayout->addWidget(m_backBtn);

    m_headerTitle = new QLabel(centralContainer);
    m_headerTitle->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: 700; padding-left: 6px; border: none; background: transparent;");
    topNavLayout->addWidget(m_headerTitle, 1);

    auto* topCopyIp = new QPushButton(tr("📋 COPY ADDRESS"), centralContainer);
    topCopyIp->setObjectName("btnSecondary");
    topCopyIp->setCursor(Qt::PointingHandCursor);
    topCopyIp->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #3F3F46; }");
    connect(topCopyIp, &QPushButton::clicked, this, [this, topCopyIp]() {
        QString addr = (m_server.port == 25565) ? m_server.ip : QString("%1:%2").arg(m_server.ip).arg(m_server.port);
        QGuiApplication::clipboard()->setText(addr);
        topCopyIp->setText(tr("✓ COPIED!"));
    });
    topNavLayout->addWidget(topCopyIp);

    mainLayout->addLayout(topNavLayout);

    // 2. Hero Banner (Full-width stretching 150px height)
    m_bannerFrame = new QFrame(centralContainer);
    m_bannerFrame->setObjectName("realmBannerFrame");
    m_bannerFrame->setAttribute(Qt::WA_StyledBackground, true);
    m_bannerFrame->setFixedHeight(150);
    m_bannerFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_bannerFrame->setStyleSheet("QFrame#realmBannerFrame { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 12px; }");

    m_bannerBgLabel = new QLabel(m_bannerFrame);
    m_bannerBgLabel->setObjectName("realmBannerBg");
    m_bannerBgLabel->setStyleSheet("background: transparent; border: none; border-radius: 12px;");
    m_bannerBgLabel->setScaledContents(true);
    m_bannerBgLabel->setGeometry(0, 0, 1144, 150);
    m_bannerBgLabel->lower();

    auto* bannerLayout = new QHBoxLayout(m_bannerFrame);
    bannerLayout->setContentsMargins(28, 24, 28, 24);

    auto* bannerTextLayout = new QVBoxLayout();
    bannerTextLayout->setSpacing(6);
    auto* bannerSub = new QLabel(tr("SUNVEIL REALM NETWORK"), m_bannerFrame);
    bannerSub->setStyleSheet("color: #00E599; font-size: 12px; font-weight: 900; letter-spacing: 1.5px; border: none; background: transparent;");
    bannerTextLayout->addWidget(bannerSub);

    auto* bannerMain = new QLabel(tr("Verified Dedicated Multiplayer Infrastructure"), m_bannerFrame);
    bannerMain->setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: 900; border: none; background: transparent;");
    bannerTextLayout->addWidget(bannerMain);
    bannerTextLayout->addStretch();
    bannerLayout->addLayout(bannerTextLayout, 1);

    m_bannerBadge = new QLabel(tr("● ONLINE & SYNCHRONIZED"), m_bannerFrame);
    m_bannerBadge->setObjectName("badgeVerified");
    m_bannerBadge->setStyleSheet("background-color: rgba(0, 229, 153, 0.12); color: #00E599; border: 1px solid rgba(0, 229, 153, 0.35); border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    bannerLayout->addWidget(m_bannerBadge, 0, Qt::AlignTop | Qt::AlignRight);

    mainLayout->addWidget(m_bannerFrame);

    // 3. 2-Column Main Content
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(18);

    // Left Column (Server Identity & Connect Panel - 340px)
    auto* leftFrame = new QFrame(centralContainer);
    leftFrame->setObjectName("leftPanelFrame");
    leftFrame->setAttribute(Qt::WA_StyledBackground, true);
    leftFrame->setFixedWidth(340);
    leftFrame->setStyleSheet("QFrame#leftPanelFrame { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 12px; }");
    auto* leftLayout = new QVBoxLayout(leftFrame);
    leftLayout->setContentsMargins(22, 22, 22, 22);
    leftLayout->setSpacing(14);

    // 120x120 Server Icon
    auto* iconContainer = new QFrame(leftFrame);
    iconContainer->setFixedSize(120, 120);
    iconContainer->setStyleSheet("background-color: #111111; border: 1px solid #2C2C2E; border-radius: 12px;");
    auto* iconInnerLayout = new QVBoxLayout(iconContainer);
    iconInnerLayout->setContentsMargins(0, 0, 0, 0);
    m_iconLabel = new QLabel(tr("⚡"), iconContainer);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet("color: #00E599; font-size: 52px; font-weight: 900; background: transparent; border: none;");
    iconInnerLayout->addWidget(m_iconLabel);
    leftLayout->addWidget(iconContainer, 0, Qt::AlignCenter);

    m_serverNameLabel = new QLabel(leftFrame);
    m_serverNameLabel->setObjectName("serverTitle");
    m_serverNameLabel->setAlignment(Qt::AlignCenter);
    m_serverNameLabel->setWordWrap(true);
    m_serverNameLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 700; background: transparent; border: none;");
    leftLayout->addWidget(m_serverNameLabel);

    // Badges row
    auto* pillsLayout = new QHBoxLayout();
    pillsLayout->setSpacing(6);
    pillsLayout->setAlignment(Qt::AlignCenter);

    m_playerCountBadge = new QLabel(leftFrame);
    m_playerCountBadge->setObjectName("playerCountBadge");
    m_playerCountBadge->setStyleSheet("background-color: #111111; color: #00E599; border: 1px solid #2C2C2E; border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    pillsLayout->addWidget(m_playerCountBadge);

    m_latencyBadge = new QLabel(tr("24ms"), leftFrame);
    m_latencyBadge->setObjectName("badgeMeta");
    m_latencyBadge->setStyleSheet("background-color: #111111; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 6px; padding: 4px 8px; font-size: 11px; font-weight: 600;");
    pillsLayout->addWidget(m_latencyBadge);

    m_regionBadge = new QLabel(tr("EU"), leftFrame);
    m_regionBadge->setObjectName("badgeMeta");
    m_regionBadge->setStyleSheet("background-color: #111111; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 6px; padding: 4px 8px; font-size: 11px; font-weight: 600;");
    pillsLayout->addWidget(m_regionBadge);
    leftLayout->addLayout(pillsLayout);

    leftLayout->addSpacing(6);

    // Primary CTA Button
    m_connectBtn = new QPushButton(tr("CONNECT TO REALM"), leftFrame);
    m_connectBtn->setObjectName("btnConnect");
    m_connectBtn->setCursor(Qt::PointingHandCursor);
    m_connectBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 13px; font-weight: 900; letter-spacing: 0.5px; border: none; border-radius: 8px; padding: 12px 24px; min-height: 24px; } QPushButton:hover { background-color: #10FFAC; color: #000000; } QPushButton:pressed { background-color: #00B377; } QPushButton:disabled { background-color: #2C2C2E; color: #52525B; }");
    connect(m_connectBtn, &QPushButton::clicked, this, [this]() {
        m_connectBtn->setEnabled(false);
        m_connectBtn->setText(tr("CONNECTING..."));
        emit connectRequested(m_server);
        if (m_connectBtn && m_server.isOnline) {
            m_connectBtn->setEnabled(true);
            m_connectBtn->setText(tr("CONNECT TO REALM"));
        }
    });
    leftLayout->addWidget(m_connectBtn);

    // Dynamic Action Link Buttons Container
    m_linksContainer = new QWidget(leftFrame);
    m_linksContainer->setStyleSheet("background: transparent; border: none;");
    m_linksLayout = new QVBoxLayout(m_linksContainer);
    m_linksLayout->setContentsMargins(0, 0, 0, 0);
    m_linksLayout->setSpacing(8);
    leftLayout->addWidget(m_linksContainer);

    leftLayout->addStretch();
    contentLayout->addWidget(leftFrame);

    // Right Column (Details, Description, and Resources - Expanding full width)
    auto* rightScroll = new QScrollArea(centralContainer);
    rightScroll->setWidgetResizable(true);
    rightScroll->setFrameShape(QFrame::NoFrame);
    rightScroll->setStyleSheet("border: none; background: transparent;");
    rightScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* rightContainer = new QWidget();
    rightContainer->setStyleSheet("background: transparent;");
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(14);

    // 1. Info Grid Card
    auto* infoCard = new QFrame(rightContainer);
    infoCard->setObjectName("rightCardFrame");
    infoCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* infoCardLayout = new QVBoxLayout(infoCard);
    infoCardLayout->setContentsMargins(18, 16, 18, 16);
    infoCardLayout->setSpacing(10);

    auto* infoTitle = new QLabel(tr("REALM SPECIFICATIONS"), infoCard);
    infoTitle->setStyleSheet("color: #00E599; font-size: 12px; font-weight: 700; letter-spacing: 0.5px; border: none;");
    infoCardLayout->addWidget(infoTitle);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(8);

    auto* ipKey = new QLabel(tr("DIRECT ADDRESS"), infoCard);
    ipKey->setStyleSheet("color: #71717A; font-size: 11px; font-weight: 600; border: none;");
    m_ipValLabel = new QLabel(infoCard);
    m_ipValLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 700; border: none;");
    grid->addWidget(ipKey, 0, 0);
    grid->addWidget(m_ipValLabel, 1, 0);

    auto* verKey = new QLabel(tr("GAME VERSION"), infoCard);
    verKey->setStyleSheet("color: #71717A; font-size: 11px; font-weight: 600; border: none;");
    m_versionValLabel = new QLabel(infoCard);
    m_versionValLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 700; border: none;");
    grid->addWidget(verKey, 0, 1);
    grid->addWidget(m_versionValLabel, 1, 1);

    auto* loaderKey = new QLabel(tr("MOD LOADER"), infoCard);
    loaderKey->setStyleSheet("color: #71717A; font-size: 11px; font-weight: 600; border: none;");
    m_loaderValLabel = new QLabel(infoCard);
    m_loaderValLabel->setStyleSheet("color: #FFFFFF; font-size: 13px; font-weight: 700; border: none;");
    grid->addWidget(loaderKey, 0, 2);
    grid->addWidget(m_loaderValLabel, 1, 2);

    auto* statusKey = new QLabel(tr("STATUS"), infoCard);
    statusKey->setStyleSheet("color: #71717A; font-size: 11px; font-weight: 600; border: none;");
    m_statusValLabel = new QLabel(infoCard);
    m_statusValLabel->setStyleSheet("color: #00E599; font-size: 13px; font-weight: 700; border: none;");
    grid->addWidget(statusKey, 0, 3);
    grid->addWidget(m_statusValLabel, 1, 3);

    infoCardLayout->addLayout(grid);
    rightLayout->addWidget(infoCard);

    // 2. Rich Markdown Description Card
    auto* aboutCard = new QFrame(rightContainer);
    aboutCard->setObjectName("rightCardFrame");
    aboutCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* aboutCardLayout = new QVBoxLayout(aboutCard);
    aboutCardLayout->setContentsMargins(18, 16, 18, 16);
    aboutCardLayout->setSpacing(8);

    auto* aboutTitle = new QLabel(tr("ABOUT THIS REALM"), aboutCard);
    aboutTitle->setStyleSheet("color: #00E599; font-size: 12px; font-weight: 700; letter-spacing: 0.5px; border: none;");
    aboutCardLayout->addWidget(aboutTitle);

    m_descBrowser = new QTextBrowser(aboutCard);
    m_descBrowser->setOpenExternalLinks(true);
    m_descBrowser->setStyleSheet("background-color: #111111; border: 1px solid #2C2C2E; border-radius: 8px; padding: 10px; color: #A1A1AA; font-size: 13px;");
    m_descBrowser->setMinimumHeight(100);
    aboutCardLayout->addWidget(m_descBrowser);
    rightLayout->addWidget(aboutCard);

    // 3. Resources / Mods Card
    auto* modsCard = new QFrame(rightContainer);
    modsCard->setObjectName("rightCardFrame");
    modsCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* modsCardLayout = new QVBoxLayout(modsCard);
    modsCardLayout->setContentsMargins(18, 16, 18, 16);
    modsCardLayout->setSpacing(10);

    m_modsHeaderLabel = new QLabel(tr("SYNCHRONIZED CLIENT RESOURCES"), modsCard);
    m_modsHeaderLabel->setStyleSheet("color: #00E599; font-size: 12px; font-weight: 700; letter-spacing: 0.5px; border: none;");
    modsCardLayout->addWidget(m_modsHeaderLabel);

    m_modsContainer = new QWidget(modsCard);
    m_modsLayout = new QVBoxLayout(m_modsContainer);
    m_modsLayout->setContentsMargins(0, 0, 0, 0);
    m_modsLayout->setSpacing(6);
    modsCardLayout->addWidget(m_modsContainer);

    rightLayout->addWidget(modsCard);
    rightLayout->addStretch();

    rightScroll->setWidget(rightContainer);
    contentLayout->addWidget(rightScroll, 1);

    mainLayout->addLayout(contentLayout, 1);

    outerLayout->addWidget(centralContainer, 10);
    outerLayout->addStretch(1);
}

void SVLRealmDetailPage::setServer(const SVLServerModel& server)
{
    m_server = server;
    updateUI();
}

void SVLRealmDetailPage::updateUI()
{
    m_headerTitle->setText(m_server.name);
    m_serverNameLabel->setText(m_server.name);

    if (m_iconLabel) {
        m_iconLabel->setPixmap(SVLConnectPage::loadServerIcon(m_server.icon, 120, 120, 12));
    }

    // Banner badge & visual styling
    if (m_server.boosts > 0) {
        m_bannerBadge->setText(QString("🔥 BOOSTED REALM (%1 BOOSTS)").arg(m_server.boosts));
        m_bannerBadge->setStyleSheet("background-color: rgba(255, 184, 0, 0.15); color: #FFB800; border: 1px solid rgba(255, 184, 0, 0.4); border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 800;");
    } else if (m_server.sponsored) {
        m_bannerBadge->setText(tr("★ SPONSORED REALM"));
        m_bannerBadge->setStyleSheet("background-color: rgba(255, 184, 0, 0.15); color: #FFB800; border: 1px solid rgba(255, 184, 0, 0.4); border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 800;");
    } else if (m_server.isOnline) {
        m_bannerBadge->setText(tr("● ONLINE & SYNCHRONIZED"));
        m_bannerBadge->setStyleSheet("background-color: rgba(0, 229, 153, 0.12); color: #00E599; border: 1px solid rgba(0, 229, 153, 0.35); border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    } else {
        m_bannerBadge->setText(tr("● OFFLINE"));
        m_bannerBadge->setStyleSheet("background-color: rgba(239, 68, 68, 0.12); color: #EF4444; border: 1px solid rgba(239, 68, 68, 0.35); border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: 700;");
    }

    loadBanner(m_server.bannerUrl);

    if (m_server.isOnline) {
        m_playerCountBadge->setVisible(true);
        m_playerCountBadge->setText(QString("👥 %1 / %2 PLAYERS").arg(m_server.players).arg(m_server.maxPlayers));
        m_connectBtn->setEnabled(true);
        m_connectBtn->setText(tr("CONNECT TO REALM"));
        m_connectBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 13px; font-weight: 900; letter-spacing: 0.5px; border: none; border-radius: 8px; padding: 12px 24px; min-height: 24px; } QPushButton:hover { background-color: #10FFAC; color: #000000; } QPushButton:pressed { background-color: #00B377; } QPushButton:disabled { background-color: #2C2C2E; color: #52525B; }");
    } else {
        m_playerCountBadge->setVisible(false);
        m_connectBtn->setEnabled(false);
        m_connectBtn->setText(tr("OFFLINE"));
        m_connectBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #71717A; font-size: 13px; font-weight: 800; border: 1px solid #3F3F46; border-radius: 8px; padding: 12px 24px; min-height: 24px; }");
    }

    // Dynamic Action Link Buttons
    QLayoutItem* linkItem;
    while ((linkItem = m_linksLayout->takeAt(0)) != nullptr) {
        if (linkItem->widget()) linkItem->widget()->deleteLater();
        delete linkItem;
    }

    // Copy IP button
    auto* copyIpBtn = new QPushButton(tr("📋 Copy Address"), m_linksContainer);
    copyIpBtn->setCursor(Qt::PointingHandCursor);
    copyIpBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #3F3F46; }");
    connect(copyIpBtn, &QPushButton::clicked, this, [this, copyIpBtn]() {
        QString addr = (m_server.port == 25565) ? m_server.ip : QString("%1:%2").arg(m_server.ip).arg(m_server.port);
        QGuiApplication::clipboard()->setText(addr);
        copyIpBtn->setText(tr("✓ Copied!"));
    });
    m_linksLayout->addWidget(copyIpBtn);

    // Store Button (Soft gold outline)
    if (!m_server.links.store.isEmpty()) {
        auto* storeBtn = new QPushButton(tr("🛒 Store & Perks"), m_linksContainer);
        storeBtn->setCursor(Qt::PointingHandCursor);
        storeBtn->setStyleSheet("QPushButton { background-color: rgba(255, 184, 0, 0.08); color: #FFB800; border: 1px solid #FFB800; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: rgba(255, 184, 0, 0.18); border-color: #FFC72C; } QPushButton:pressed { background-color: rgba(255, 184, 0, 0.28); }");
        connect(storeBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl(m_server.links.store));
        });
        m_linksLayout->addWidget(storeBtn);
    }

    // Discord Button (Blurple #5865F2)
    if (!m_server.links.discord.isEmpty()) {
        auto* discordBtn = new QPushButton(tr("💬 Discord Community"), m_linksContainer);
        discordBtn->setCursor(Qt::PointingHandCursor);
        discordBtn->setStyleSheet("QPushButton { background-color: rgba(88, 101, 242, 0.15); color: #FFFFFF; border: 1px solid #5865F2; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: rgba(88, 101, 242, 0.3); border-color: #7289DA; } QPushButton:pressed { background-color: rgba(88, 101, 242, 0.4); }");
        connect(discordBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl(m_server.links.discord));
        });
        m_linksLayout->addWidget(discordBtn);
    }

    // Website Button (Slate outline #2C2C2E)
    if (!m_server.links.website.isEmpty()) {
        auto* websiteBtn = new QPushButton(tr("🌐 Official Website"), m_linksContainer);
        websiteBtn->setCursor(Qt::PointingHandCursor);
        websiteBtn->setStyleSheet("QPushButton { background-color: #1C1C1E; color: #E4E4E7; border: 1px solid #2C2C2E; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #27272A; border-color: #3F3F46; color: #FFFFFF; } QPushButton:pressed { background-color: #18181B; }");
        connect(websiteBtn, &QPushButton::clicked, this, [this]() {
            QDesktopServices::openUrl(QUrl(m_server.links.website));
        });
        m_linksLayout->addWidget(websiteBtn);
    }

    QString addr = (m_server.port == 25565) ? m_server.ip : QString("%1:%2").arg(m_server.ip).arg(m_server.port);
    m_ipValLabel->setText(addr);
    m_versionValLabel->setText(QString("Minecraft %1").arg(m_server.mcVersion));
    m_loaderValLabel->setText(QString("%1 %2").arg(m_server.loader.toUpper(), m_server.loaderVersion.isEmpty() ? "52.0.18" : m_server.loaderVersion));
    m_statusValLabel->setText(m_server.verified ? tr("Verified Official") : tr("Community Hosted"));

    QString descText = m_server.motd.isEmpty() ? tr("Welcome to %1! Automatic mod synchronization is active for this server.").arg(m_server.name) : m_server.motd;
    m_descBrowser->setMarkdown(descText);

    m_modsHeaderLabel->setText(tr("SYNCHRONIZED CLIENT RESOURCES (%1 MODS)").arg(m_server.modCount));

    // Clear and rebuild mod items
    QLayoutItem* item;
    while ((item = m_modsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (m_server.modCount == 0) {
        auto* noMods = new QLabel(tr("No client mods required. Vanilla compatible."), m_modsContainer);
        noMods->setStyleSheet("color: #71717A; font-size: 12px; padding: 4px; border: none;");
        m_modsLayout->addWidget(noMods);
    } else {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        auto* bridgeBadge = new QLabel(tr("⚡ Sunveil Bridge"), m_modsContainer);
        bridgeBadge->setObjectName("badgeVerified");
        row->addWidget(bridgeBadge);

        auto* modsCountBadge = new QLabel(tr("📦 %1 Dynamic Mod Assets [Modrinth / CurseForge]").arg(m_server.modCount), m_modsContainer);
        modsCountBadge->setObjectName("badgeMeta");
        row->addWidget(modsCountBadge);
        row->addStretch();
        m_modsLayout->addLayout(row);
    }
}

void SVLRealmDetailPage::loadBanner(const QString& bannerUrl)
{
    if (bannerUrl.isEmpty()) {
        if (m_bannerBgLabel) {
            m_bannerBgLabel->clear();
        }
        return;
    }

    QUrl url(bannerUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    SVLSecurity::injectAuthHeaders(req);

    auto* reply = APPLICATION->network()->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, bannerUrl]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError && m_server.bannerUrl == bannerUrl) {
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data)) {
                applyBannerPixmap(pix);
            }
        }
    });
}

void SVLRealmDetailPage::applyBannerPixmap(const QPixmap& originalPixmap)
{
    if (originalPixmap.isNull() || !m_bannerBgLabel) {
        return;
    }

    int targetWidth = m_bannerFrame ? m_bannerFrame->width() : 1144;
    if (targetWidth <= 0) {
        targetWidth = 1144;
    }
    int targetHeight = 150;

    m_bannerBgLabel->resize(targetWidth, targetHeight);

    QPixmap roundedBanner(targetWidth, targetHeight);
    roundedBanner.fill(Qt::transparent);

    QPainter painter(&roundedBanner);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath clipPath;
    clipPath.addRoundedRect(0, 0, targetWidth, targetHeight, 12, 12);
    painter.setClipPath(clipPath);

    // Scale image covering target rectangle
    QPixmap scaled = originalPixmap.scaled(targetWidth, targetHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (scaled.width() - targetWidth) / 2;
    int y = (scaled.height() - targetHeight) / 2;
    painter.drawPixmap(0, 0, scaled, x, y, targetWidth, targetHeight);

    // Dark overlay gradient for maximum text legibility
    QLinearGradient grad(0, 0, targetWidth, 0);
    grad.setColorAt(0.0, QColor(17, 17, 19, 235));
    grad.setColorAt(0.5, QColor(17, 17, 19, 190));
    grad.setColorAt(1.0, QColor(17, 17, 19, 140));
    painter.fillRect(0, 0, targetWidth, targetHeight, grad);
    painter.end();

    m_bannerBgLabel->setPixmap(roundedBanner);
}

