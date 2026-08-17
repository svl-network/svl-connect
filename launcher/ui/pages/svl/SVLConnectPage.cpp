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
#include "ui/dialogs/ProgressDialog.h"
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
    setStyleSheet(R"(
        QWidget#SVLConnectPage {
            background-color: #080C0E;
            color: #F8FAFC;
            font-family: 'Segoe UI', -apple-system, sans-serif;
        }
        QLineEdit#searchEdit {
            background-color: #0E1418;
            border: 1px solid #1E2B33;
            border-radius: 8px;
            padding: 10px 16px;
            color: #F8FAFC;
            font-size: 13px;
        }
        QLineEdit#searchEdit:focus {
            border: 1px solid #00E599;
            background-color: #121A20;
        }
        QPushButton#refreshButton {
            background-color: #0E1418;
            border: 1px solid #1E2B33;
            border-radius: 8px;
            padding: 10px 20px;
            color: #00E599;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton#refreshButton:hover {
            background-color: #151F26;
            border-color: #00E599;
            color: #00FFAC;
        }
        QPushButton#refreshButton:pressed {
            background-color: #0E1418;
        }
        QScrollArea {
            border: none;
            background-color: transparent;
        }
        QFrame[serverCard="true"] {
            background-color: #0E1418;
            border: 1px solid #1E2B33;
            border-radius: 12px;
        }
        QFrame[serverCard="true"]:hover {
            border: 1px solid #00E599;
            background-color: #121A20;
        }
        QLabel.serverTitle {
            font-size: 17px;
            font-weight: 700;
            color: #FFB800;
        }
        QLabel.playerCount {
            font-size: 13px;
            font-weight: 600;
            color: #94A3B8;
        }
        QLabel.serverMotd {
            font-size: 13px;
            color: #94A3B8;
            line-height: 1.4;
        }
        QLabel.badgeTag {
            background-color: #151F26;
            color: #94A3B8;
            border: 1px solid #233440;
            border-radius: 10px;
            padding: 3px 10px;
            font-size: 11px;
            font-weight: bold;
        }
        QLabel.verifiedBadge {
            background-color: rgba(0, 229, 153, 0.12);
            color: #00E599;
            border: 1px solid #00E599;
            border-radius: 10px;
            padding: 3px 10px;
            font-size: 11px;
            font-weight: bold;
        }
        QLabel.communityBadge {
            background-color: rgba(245, 158, 11, 0.12);
            color: #F59E0B;
            border: 1px solid #F59E0B;
            border-radius: 10px;
            padding: 3px 10px;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton.connectButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E599, stop:1 #00C480);
            color: #080C0E;
            border: none;
            border-radius: 8px;
            padding: 9px 24px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton.connectButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00FFAC, stop:1 #00E599);
            color: #080C0E;
        }
        QPushButton.connectButton:pressed {
            background: #00C480;
        }
    )");

    setObjectName("SVLConnectPage");
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // Header Controls
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText(tr("🔍 Filter server realms by name, version, or MOTD..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SVLConnectPage::onSearchFilterChanged);
    headerLayout->addWidget(m_searchEdit, 1);

    auto* refreshBtn = new QPushButton(tr("🔄 Refresh"), this);
    refreshBtn->setObjectName("refreshButton");
    connect(refreshBtn, &QPushButton::clicked, this, &SVLConnectPage::refreshServers);
    headerLayout->addWidget(refreshBtn);

    mainLayout->addLayout(headerLayout);

    // Status / Count bar
    m_statusLabel = new QLabel(tr("Fetching active servers from Sunveil Network..."), this);
    m_statusLabel->setStyleSheet("color: #64748B; font-size: 12px; font-weight: bold;");
    mainLayout->addWidget(m_statusLabel);

    // Scroll Area for Server Cards
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

    m_statusLabel->setText(tr("Connecting to %1...").arg(m_masterApiBaseUrl));

    QUrl url(m_masterApiBaseUrl + "/api/v1/servers");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(6000);

    m_currentReply = APPLICATION->network()->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &SVLConnectPage::onServersReceived);
}

void SVLConnectPage::onServersReceived()
{
    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText(tr("⚠️ Failed to reach Master API (%1). Check connection or host settings.").arg(reply->errorString()));
        m_allServers.clear();
        renderServerCards();
        return;
    }

    QByteArray payload = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

    m_allServers.clear();
    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const QJsonValue& val : arr) {
            if (!val.isObject()) continue;
            QJsonObject obj = val.toObject();

            SVLServerModel model;
            model.serverKey = obj.value("serverKey").toString();
            model.name = obj.value("name").toString(model.serverKey);
            model.ip = obj.value("ip").toString("127.0.0.1");
            model.port = static_cast<quint16>(obj.value("port").toInt(25565));
            model.verified = obj.value("verified").toBool(false);

            QJsonObject verObj = obj.value("version").toObject();
            model.mcVersion = verObj.value("minecraft").toString("1.21.1");
            model.loader = verObj.value("loader").toString("fabric");
            model.loaderVersion = verObj.value("loaderVersion").toString();

            QJsonObject statObj = obj.value("status").toObject();
            model.players = statObj.value("players").toInt(0);
            model.maxPlayers = statObj.value("maxPlayers").toInt(20);
            model.motd = statObj.value("motd").toString();

            QJsonArray modsArr = obj.value("mods").toArray();
            model.modCount = modsArr.size();

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

    m_statusLabel->setText(tr("Online Realms: %1 active server(s) found.").arg(m_filteredServers.size()));
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
        auto* emptyLabel = new QLabel(tr("No active Sunveil servers online. Click 'Refresh' to poll again."), this);
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
    card->setProperty("serverCard", true);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 16);
    cardLayout->setSpacing(10);

    // 1. Top Row: Server Title on left, Player Count on right
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    auto* nameLabel = new QLabel(server.name, card);
    nameLabel->setProperty("class", "serverTitle");
    topRow->addWidget(nameLabel, 1);

    auto* playersLabel = new QLabel(QString("🟢 %1 / %2").arg(QString::number(server.players), QString::number(server.maxPlayers)), card);
    playersLabel->setProperty("class", "playerCount");
    playersLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    topRow->addWidget(playersLabel);

    cardLayout->addLayout(topRow);

    // 2. Middle Row: MOTD Description
    if (!server.motd.isEmpty()) {
        auto* motdLabel = new QLabel(server.motd, card);
        motdLabel->setProperty("class", "serverMotd");
        motdLabel->setWordWrap(true);
        cardLayout->addWidget(motdLabel);
    }

    // 3. Bottom Row: Badges on left, Connect CTA button on right
    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);

    // Verified / Community badge
    auto* statusBadge = new QLabel(card);
    if (server.verified) {
        statusBadge->setText(tr("🟢 Verified"));
        statusBadge->setProperty("class", "verifiedBadge");
    } else {
        statusBadge->setText(tr("🟡 Community"));
        statusBadge->setProperty("class", "communityBadge");
    }
    bottomRow->addWidget(statusBadge);

    // Loader & Version badge
    auto* loaderBadge = new QLabel(QString("%1 %2").arg(server.loader.toUpper(), server.mcVersion), card);
    loaderBadge->setProperty("class", "badgeTag");
    bottomRow->addWidget(loaderBadge);

    // Mods count badge
    auto* modsBadge = new QLabel(tr("📦 %1 Mods").arg(server.modCount), card);
    modsBadge->setProperty("class", "badgeTag");
    bottomRow->addWidget(modsBadge);

    bottomRow->addStretch(1);

    // Connect button
    auto* connectBtn = new QPushButton(tr("⚡ JETZT SPIELEN"), card);
    connectBtn->setProperty("class", "connectButton");
    connect(connectBtn, &QPushButton::clicked, this, [this, server]() {
        onConnectClicked(server);
    });
    bottomRow->addWidget(connectBtn);

    cardLayout->addLayout(bottomRow);

    return card;
}

void SVLConnectPage::onConnectClicked(const SVLServerModel& server)
{
    auto* syncTask = new SVLModSyncTask(m_masterApiBaseUrl, server.serverKey, server.name, server.ip, server.port, this);

    connect(syncTask, &SVLModSyncTask::readyToLaunch, this, [this](MinecraftInstance* inst, const QString& ip, quint16 port) {
        emit launchRequested(inst, ip, port);
    });

    ProgressDialog dlg(this);
    dlg.setWindowTitle(tr("Sunveil Mod Synchronization"));
    if (dlg.execWithTask(syncTask) != QDialog::Accepted) {
        qDebug() << "[SVLConnectPage] Mod sync task cancelled or aborted.";
    }
}
