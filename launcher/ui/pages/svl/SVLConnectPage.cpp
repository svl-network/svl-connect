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
#include <QMessageBox>

#include "Application.h"
#include "tasks/SVLModSyncTask.h"
#include "ui/dialogs/SVLLoadingOverlay.h"
#include "ui/dialogs/CustomMessageBox.h"
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

void SVLConnectPage::setupUI()
{
    setObjectName("SVLConnectPage");
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // 1. Clean Page Header
    auto* pageHeaderLayout = new QVBoxLayout();
    pageHeaderLayout->setSpacing(4);

    auto* titleLabel = new QLabel(tr("Realms"), this);
    titleLabel->setObjectName("pageTitleLabel");
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: 700;");
    pageHeaderLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(tr("Select a server to automatically synchronize mods and connect."), this);
    subtitleLabel->setObjectName("pageSubtitleLabel");
    subtitleLabel->setStyleSheet("color: #94A3B8; font-size: 13px;");
    pageHeaderLayout->addWidget(subtitleLabel);

    mainLayout->addLayout(pageHeaderLayout);

    // 2. Filter & Controls Bar
    auto* filterFrame = new QFrame(this);
    filterFrame->setObjectName("filterBarFrame");
    filterFrame->setAttribute(Qt::WA_StyledBackground, true);
    auto* filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(12, 8, 12, 8);
    filterLayout->setSpacing(12);

    m_searchEdit = new QLineEdit(filterFrame);
    m_searchEdit->setObjectName("realmSearchInput");
    m_searchEdit->setPlaceholderText(tr("Search realms..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SVLConnectPage::onSearchFilterChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    m_statusLabel = new QLabel(tr("0 Online"), filterFrame);
    m_statusLabel->setObjectName("statusPillBadge");
    filterLayout->addWidget(m_statusLabel);

    auto* refreshBtn = new QPushButton(tr("Refresh"), filterFrame);
    refreshBtn->setObjectName("refreshButton");
    refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(refreshBtn, &QPushButton::clicked, this, &SVLConnectPage::refreshServers);
    filterLayout->addWidget(refreshBtn);

    mainLayout->addWidget(filterFrame);

    // 3. Scroll Area for Server Cards
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    auto* scrollContainer = new QWidget();
    scrollContainer->setStyleSheet("background-color: transparent;");
    m_cardsLayout = new QVBoxLayout(scrollContainer);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(12);
    m_cardsLayout->addStretch();

    m_scrollArea->setWidget(scrollContainer);
    mainLayout->addWidget(m_scrollArea, 1);
}

void SVLConnectPage::refreshServers()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    m_statusLabel->setText(tr("Connecting..."));

    QUrl url(m_masterApiBaseUrl + "/api/v1/servers");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(6000);

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

    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText(tr("Offline"));
        m_allServers.clear();
        m_filteredServers.clear();
        renderServerCards();
        return;
    }

    QByteArray responsePayload = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responsePayload);
    if (!doc.isArray()) {
        m_statusLabel->setText(tr("Invalid Data"));
        return;
    }

    m_allServers.clear();
    QJsonArray array = doc.array();
    for (const auto& val : array) {
        if (val.isObject()) {
            QJsonObject obj = val.toObject();
            SVLServerModel model;
            model.serverKey = obj.value("serverKey").toString();
            model.name = obj.value("name").toString(model.serverKey);
            model.ip = obj.value("ip").toString("127.0.0.1");
            model.port = static_cast<quint16>(obj.value("port").toInt(25565));

            QJsonObject verObj = obj.value("version").toObject();
            model.mcVersion = verObj.value("minecraft").toString(obj.value("mcVersion").toString("1.21.1"));
            model.loader = verObj.value("loader").toString(obj.value("loader").toString("fabric"));
            model.loaderVersion = verObj.value("loaderVersion").toString(obj.value("loaderVersion").toString());

            QJsonObject statObj = obj.value("status").toObject();
            model.players = statObj.value("players").toInt(obj.value("players").toInt(0));
            model.maxPlayers = statObj.value("maxPlayers").toInt(obj.value("maxPlayers").toInt(20));
            model.motd = statObj.value("motd").toString(obj.value("motd").toString());
            model.verified = obj.value("verified").toBool(false);

            if (obj.contains("mods") && obj.value("mods").isArray()) {
                model.modCount = obj.value("mods").toArray().size();
            } else {
                model.modCount = obj.value("modCount").toInt(0);
            }
            m_allServers.append(model);
        }
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

    m_statusLabel->setText(tr("● %1 Available").arg(m_filteredServers.size()));
    renderServerCards();
}

void SVLConnectPage::renderServerCards()
{
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
        emptyLabel->setStyleSheet("color: #64748B; font-size: 14px; padding: 40px;");
        m_cardsLayout->addWidget(emptyLabel);
        m_cardsLayout->addStretch();
        return;
    }

    for (const auto& server : m_filteredServers) {
        m_cardsLayout->addWidget(createServerCard(server));
    }
    m_cardsLayout->addStretch();
}

QWidget* SVLConnectPage::createServerCard(const SVLServerModel& server)
{
    auto* card = new QFrame(this);
    card->setObjectName("serverCard");
    card->setAttribute(Qt::WA_StyledBackground, true);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(10);

    // 1. Top row: Name & Player count
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    auto* nameLabel = new QLabel(server.name, card);
    nameLabel->setObjectName("serverNameLabel");
    topRow->addWidget(nameLabel, 1);

    auto* playersLabel = new QLabel(QString("%1 / %2").arg(QString::number(server.players), QString::number(server.maxPlayers)), card);
    playersLabel->setObjectName("playerCountBadge");
    playersLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(playersLabel);

    cardLayout->addLayout(topRow);

    // 2. MOTD (if present)
    if (!server.motd.isEmpty()) {
        auto* motdLabel = new QLabel(server.motd, card);
        motdLabel->setObjectName("serverMotdLabel");
        motdLabel->setWordWrap(true);
        cardLayout->addWidget(motdLabel);
    }

    // 3. Bottom row: Specifications & Actions
    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);

    if (server.verified) {
        auto* statusBadge = new QLabel(tr("Verified"), card);
        statusBadge->setObjectName("badgeVerified");
        bottomRow->addWidget(statusBadge);
    }

    auto* loaderBadge = new QLabel(QString("%1 %2").arg(server.loader.toUpper(), server.mcVersion), card);
    loaderBadge->setProperty("class", "metaPill");
    bottomRow->addWidget(loaderBadge);

    if (server.modCount > 0) {
        auto* modsBadge = new QLabel(tr("%1 mods").arg(server.modCount), card);
        modsBadge->setProperty("class", "metaPill");
        bottomRow->addWidget(modsBadge);
    }

    bottomRow->addStretch(1);

    auto* detailsBtn = new QPushButton(tr("Details"), card);
    detailsBtn->setObjectName("cardDetailsBtn");
    detailsBtn->setCursor(Qt::PointingHandCursor);
    connect(detailsBtn, &QPushButton::clicked, this, [this, server]() {
        emit serverDetailsRequested(server);
    });
    bottomRow->addWidget(detailsBtn);

    auto* connectBtn = new QPushButton(tr("Connect"), card);
    connectBtn->setObjectName("joinServerButton");
    connectBtn->setCursor(Qt::PointingHandCursor);
    connect(connectBtn, &QPushButton::clicked, this, [this, server]() {
        onConnectClicked(server);
    });
    bottomRow->addWidget(connectBtn);

    cardLayout->addLayout(bottomRow);
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

    connect(syncTask, &Task::failed, overlay, [this, overlay](const QString& reason) {
        overlay->reject();
        CustomMessageBox::selectable(this, tr("Connection Failed"), reason, QMessageBox::Critical)->show();
    });

    connect(overlay, &SVLLoadingOverlay::cancelRequested, syncTask, [syncTask]() {
        syncTask->abort();
    });

    syncTask->start();
    overlay->exec();
    overlay->deleteLater();
}
