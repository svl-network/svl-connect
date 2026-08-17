#include "SVLRealmDetailPage.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QGridLayout>

SVLRealmDetailPage::SVLRealmDetailPage(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SVLRealmDetailPage::setupUI()
{
    setObjectName("SVLRealmDetailPage");
    setAttribute(Qt::WA_StyledBackground, true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // 1. Top navigation strip
    auto* topNavLayout = new QHBoxLayout();
    topNavLayout->setSpacing(12);

    m_backBtn = new QPushButton(tr("← Back"), this);
    m_backBtn->setObjectName("detailBackButton");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    connect(m_backBtn, &QPushButton::clicked, this, &SVLRealmDetailPage::backRequested);
    topNavLayout->addWidget(m_backBtn);

    m_headerTitle = new QLabel(this);
    m_headerTitle->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: 700; padding-left: 6px;");
    topNavLayout->addWidget(m_headerTitle, 1);

    mainLayout->addLayout(topNavLayout);

    // 2. 2-Column Main Content
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    // Left Column (Server Identity & Connect Panel)
    auto* leftFrame = new QFrame(this);
    leftFrame->setObjectName("leftPanelFrame");
    leftFrame->setAttribute(Qt::WA_StyledBackground, true);
    leftFrame->setFixedWidth(300);
    auto* leftLayout = new QVBoxLayout(leftFrame);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);

    m_serverNameLabel = new QLabel(leftFrame);
    m_serverNameLabel->setObjectName("detailServerName");
    m_serverNameLabel->setWordWrap(true);
    leftLayout->addWidget(m_serverNameLabel);

    // Pills row
    auto* pillsLayout = new QHBoxLayout();
    pillsLayout->setSpacing(6);

    m_playerCountBadge = new QLabel(leftFrame);
    m_playerCountBadge->setObjectName("playerCountBadge");
    m_playerCountBadge->setProperty("class", "detailPill");
    pillsLayout->addWidget(m_playerCountBadge);

    m_latencyBadge = new QLabel(tr("24ms"), leftFrame);
    m_latencyBadge->setObjectName("latencyBadge");
    m_latencyBadge->setProperty("class", "detailPill");
    pillsLayout->addWidget(m_latencyBadge);

    m_regionBadge = new QLabel(tr("EU"), leftFrame);
    m_regionBadge->setObjectName("regionBadge");
    m_regionBadge->setProperty("class", "detailPill");
    pillsLayout->addWidget(m_regionBadge);
    pillsLayout->addStretch();
    leftLayout->addLayout(pillsLayout);

    leftLayout->addSpacing(8);

    // Primary CTA Button
    m_connectBtn = new QPushButton(tr("Connect"), leftFrame);
    m_connectBtn->setObjectName("detailConnectBtn");
    m_connectBtn->setCursor(Qt::PointingHandCursor);
    connect(m_connectBtn, &QPushButton::clicked, this, [this]() {
        emit connectRequested(m_server);
    });
    leftLayout->addWidget(m_connectBtn);

    // Secondary buttons
    auto* secBtnLayout = new QHBoxLayout();
    secBtnLayout->setSpacing(6);

    m_copyIpBtn = new QPushButton(tr("Copy Address"), leftFrame);
    m_copyIpBtn->setProperty("class", "secBtn");
    m_copyIpBtn->setCursor(Qt::PointingHandCursor);
    connect(m_copyIpBtn, &QPushButton::clicked, this, [this]() {
        QString addr = (m_server.port == 25565) ? m_server.ip : QString("%1:%2").arg(m_server.ip).arg(m_server.port);
        QGuiApplication::clipboard()->setText(addr);
        m_copyIpBtn->setText(tr("Copied!"));
    });
    secBtnLayout->addWidget(m_copyIpBtn);

    m_favBtn = new QPushButton(tr("⭐ Favorite"), leftFrame);
    m_favBtn->setProperty("class", "secBtn");
    m_favBtn->setCursor(Qt::PointingHandCursor);
    secBtnLayout->addWidget(m_favBtn);

    m_discordBtn = new QPushButton(tr("💬 Discord"), leftFrame);
    m_discordBtn->setProperty("class", "secBtn");
    m_discordBtn->setCursor(Qt::PointingHandCursor);
    connect(m_discordBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://discord.gg/sunveil"));
    });
    secBtnLayout->addWidget(m_discordBtn);

    leftLayout->addLayout(secBtnLayout);
    leftLayout->addStretch();
    contentLayout->addWidget(leftFrame);

    // Right Column (Details, Description, and Resources)
    auto* rightScroll = new QScrollArea(this);
    rightScroll->setWidgetResizable(true);
    rightScroll->setStyleSheet("border: none; background: transparent;");

    auto* rightContainer = new QWidget();
    rightContainer->setStyleSheet("background: transparent;");
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(16);

    // 1. Info Grid Card
    auto* infoCard = new QFrame(rightContainer);
    infoCard->setObjectName("rightCardFrame");
    infoCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* infoCardLayout = new QVBoxLayout(infoCard);
    infoCardLayout->setContentsMargins(20, 16, 20, 16);
    infoCardLayout->setSpacing(12);

    auto* infoTitle = new QLabel(tr("📊 REALM SPECIFICATIONS"), infoCard);
    infoTitle->setProperty("class", "sectionTitle");
    infoCardLayout->addWidget(infoTitle);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);

    auto* ipKey = new QLabel(tr("DIRECT ADDRESS"), infoCard);
    ipKey->setProperty("class", "keyLabel");
    m_ipValLabel = new QLabel(infoCard);
    m_ipValLabel->setProperty("class", "valLabel");
    grid->addWidget(ipKey, 0, 0);
    grid->addWidget(m_ipValLabel, 1, 0);

    auto* verKey = new QLabel(tr("GAME VERSION"), infoCard);
    verKey->setProperty("class", "keyLabel");
    m_versionValLabel = new QLabel(infoCard);
    m_versionValLabel->setProperty("class", "valLabel");
    grid->addWidget(verKey, 0, 1);
    grid->addWidget(m_versionValLabel, 1, 1);

    auto* loaderKey = new QLabel(tr("MOD LOADER"), infoCard);
    loaderKey->setProperty("class", "keyLabel");
    m_loaderValLabel = new QLabel(infoCard);
    m_loaderValLabel->setProperty("class", "valLabel");
    grid->addWidget(loaderKey, 0, 2);
    grid->addWidget(m_loaderValLabel, 1, 2);

    auto* statusKey = new QLabel(tr("SECURITY STATUS"), infoCard);
    statusKey->setProperty("class", "keyLabel");
    m_statusValLabel = new QLabel(infoCard);
    m_statusValLabel->setProperty("class", "valLabel");
    grid->addWidget(statusKey, 0, 3);
    grid->addWidget(m_statusValLabel, 1, 3);

    infoCardLayout->addLayout(grid);
    rightLayout->addWidget(infoCard);

    // 2. MOTD / About Card
    auto* aboutCard = new QFrame(rightContainer);
    aboutCard->setObjectName("rightCardFrame");
    aboutCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* aboutCardLayout = new QVBoxLayout(aboutCard);
    aboutCardLayout->setContentsMargins(20, 16, 20, 16);
    aboutCardLayout->setSpacing(10);

    auto* aboutTitle = new QLabel(tr("📜 ABOUT THIS REALM"), aboutCard);
    aboutTitle->setProperty("class", "sectionTitle");
    aboutCardLayout->addWidget(aboutTitle);

    m_motdTextLabel = new QLabel(aboutCard);
    m_motdTextLabel->setStyleSheet("color: #CBD5E1; font-size: 13px; line-height: 1.5;");
    m_motdTextLabel->setWordWrap(true);
    aboutCardLayout->addWidget(m_motdTextLabel);
    rightLayout->addWidget(aboutCard);

    // 3. Resources / Mods Card
    auto* modsCard = new QFrame(rightContainer);
    modsCard->setObjectName("rightCardFrame");
    modsCard->setAttribute(Qt::WA_StyledBackground, true);
    auto* modsCardLayout = new QVBoxLayout(modsCard);
    modsCardLayout->setContentsMargins(20, 16, 20, 16);
    modsCardLayout->setSpacing(12);

    m_modsHeaderLabel = new QLabel(tr("📦 SYNCHRONIZED CLIENT RESOURCES"), modsCard);
    m_modsHeaderLabel->setProperty("class", "sectionTitle");
    modsCardLayout->addWidget(m_modsHeaderLabel);

    m_modsContainer = new QWidget(modsCard);
    m_modsLayout = new QVBoxLayout(m_modsContainer);
    m_modsLayout->setContentsMargins(0, 0, 0, 0);
    m_modsLayout->setSpacing(8);
    modsCardLayout->addWidget(m_modsContainer);

    rightLayout->addWidget(modsCard);
    rightLayout->addStretch();

    rightScroll->setWidget(rightContainer);
    contentLayout->addWidget(rightScroll, 1);

    mainLayout->addLayout(contentLayout, 1);
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
    m_playerCountBadge->setText(QString("🟢 %1 / %2 PLAYERS").arg(m_server.players).arg(m_server.maxPlayers));

    QString addr = (m_server.port == 25565) ? m_server.ip : QString("%1:%2").arg(m_server.ip).arg(m_server.port);
    m_ipValLabel->setText(addr);
    m_versionValLabel->setText(QString("Minecraft %1").arg(m_server.mcVersion));
    m_loaderValLabel->setText(QString("%1 (Auto-Synced)").arg(m_server.loader.toUpper()));
    m_statusValLabel->setText(m_server.verified ? tr("🟢 Verified Official") : tr("🟡 Community Hosted"));

    if (m_server.motd.isEmpty()) {
        m_motdTextLabel->setText(tr("Welcome to %1! Connect now to explore custom features, economy, and quests.").arg(m_server.name));
    } else {
        m_motdTextLabel->setText(m_server.motd);
    }

    m_modsHeaderLabel->setText(tr("📦 SYNCHRONIZED CLIENT RESOURCES (%1 MODS)").arg(m_server.modCount));

    // Clear and rebuild mod items
    QLayoutItem* item;
    while ((item = m_modsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (m_server.modCount == 0) {
        auto* noMods = new QLabel(tr("No client mods required. Vanilla compatible."), m_modsContainer);
        noMods->setStyleSheet("color: #64748B; font-size: 12px; padding: 6px;");
        m_modsLayout->addWidget(noMods);
    } else {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        auto* bridgeBadge = new QLabel(tr("⚡ Sunveil Bridge Sync"), m_modsContainer);
        bridgeBadge->setStyleSheet("color: #00E599; background: #141E26; border: 1px solid rgba(0, 229, 153, 0.3); border-radius: 6px; padding: 6px 12px; font-weight: 700; font-size: 12px;");
        row->addWidget(bridgeBadge);

        auto* modsCountBadge = new QLabel(tr("📦 %1 Dynamic Mod Assets [MODRINTH/CURSE]").arg(m_server.modCount), m_modsContainer);
        modsCountBadge->setStyleSheet("color: #CBD5E1; background: #182430; border: 1px solid #273A4D; border-radius: 6px; padding: 6px 12px; font-weight: 600; font-size: 12px;");
        row->addWidget(modsCountBadge);
        row->addStretch();
        m_modsLayout->addLayout(row);
    }
}
