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
            background-color: #11111B;
            color: #CDD6F4;
            font-family: 'Segoe UI', Roboto, sans-serif;
        }
        QLineEdit#searchEdit {
            background-color: #1E1E2E;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 8px 14px;
            color: #CDD6F4;
            font-size: 13px;
        }
        QLineEdit#searchEdit:focus {
            border: 1px solid #F59E0B;
        }
        QPushButton#refreshButton {
            background-color: #1E1E2E;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 8px 16px;
            color: #CDD6F4;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton#refreshButton:hover {
            background-color: #313244;
            border-color: #45475A;
        }
        QScrollArea {
            border: none;
            background-color: transparent;
        }
        QFrame.serverCard {
            background-color: #181825;
            border: 1px solid #313244;
            border-radius: 8px;
            padding: 12px;
        }
        QFrame.serverCard:hover {
            border: 1px solid #45475A;
            background-color: #1E1E2E;
        }
        QLabel.serverTitle {
            font-size: 16px;
            font-weight: bold;
            color: #F59E0B;
        }
        QLabel.serverMotd {
            font-size: 12px;
            color: #A6ADC8;
        }
        QLabel.badgeTag {
            background-color: #313244;
            color: #BAC2DE;
            border-radius: 4px;
            padding: 3px 8px;
            font-size: 11px;
            font-weight: bold;
        }
        QLabel.verifiedBadge {
            background-color: #064E3B;
            color: #10B981;
            border: 1px solid #059669;
            border-radius: 4px;
            padding: 3px 8px;
            font-size: 11px;
            font-weight: bold;
        }
        QLabel.communityBadge {
            background-color: #78350F;
            color: #F59E0B;
            border: 1px solid #D97706;
            border-radius: 4px;
            padding: 3px 8px;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton.connectButton {
            background-color: #F59E0B;
            color: #11111B;
            border: none;
            border-radius: 6px;
            padding: 9px 22px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton.connectButton:hover {
            background-color: #D97706;
        }
    )");

    setObjectName("SVLConnectPage");
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

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
    m_statusLabel->setStyleSheet("color: #6C7086; font-size: 12px; font-weight: bold;");
    mainLayout->addWidget(m_statusLabel);

    // Scroll Area for Server Cards
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    auto* scrollContainer = new QWidget();
    scrollContainer->setStyleSheet("background-color: transparent;");
    m_cardsLayout = new QVBoxLayout(scrollContainer);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(10);
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

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

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
        emptyLabel->setStyleSheet("color: #585B70; font-size: 14px; padding: 40px;");
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
    card->setProperty("class", "serverCard");

    auto* cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(16);

    // Left info block
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(5);

    auto* nameLabel = new QLabel(server.name, card);
    nameLabel->setProperty("class", "serverTitle");
    infoLayout->addWidget(nameLabel);

    if (!server.motd.isEmpty()) {
        auto* motdLabel = new QLabel(server.motd, card);
        motdLabel->setProperty("class", "serverMotd");
        motdLabel->setWordWrap(true);
        infoLayout->addWidget(motdLabel);
    }

    // Badges Row
    auto* badgesLayout = new QHBoxLayout();
    badgesLayout->setSpacing(8);

    auto* loaderBadge = new QLabel(QString("%1 %2").arg(server.loader.toUpper(), server.mcVersion), card);
    loaderBadge->setProperty("class", "badgeTag");
    badgesLayout->addWidget(loaderBadge);

    auto* modsBadge = new QLabel(tr("📦 %1 Mods").arg(server.modCount), card);
    modsBadge->setProperty("class", "badgeTag");
    badgesLayout->addWidget(modsBadge);

    auto* statusBadge = new QLabel(card);
    if (server.verified) {
        statusBadge->setText(tr("🟢 Verified (Official Modrinth)"));
        statusBadge->setProperty("class", "verifiedBadge");
    } else {
        statusBadge->setText(tr("🟡 Community Content"));
        statusBadge->setProperty("class", "communityBadge");
    }
    badgesLayout->addWidget(statusBadge);

    badgesLayout->addStretch();
    infoLayout->addLayout(badgesLayout);

    cardLayout->addLayout(infoLayout, 1);

    // Right Action block
    auto* actionLayout = new QVBoxLayout();
    actionLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    actionLayout->setSpacing(6);

    auto* playersLabel = new QLabel(tr("👥 %1 / %2").arg(QString::number(server.players), QString::number(server.maxPlayers)), card);
    playersLabel->setStyleSheet("color: #BAC2DE; font-weight: bold; font-size: 13px;");
    playersLabel->setAlignment(Qt::AlignRight);
    actionLayout->addWidget(playersLabel);

    auto* connectBtn = new QPushButton(tr("⚡ Connect"), card);
    connectBtn->setProperty("class", "connectButton");
    connect(connectBtn, &QPushButton::clicked, this, [this, server]() {
        onConnectClicked(server);
    });
    actionLayout->addWidget(connectBtn);

    cardLayout->addLayout(actionLayout);

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
    if (dlg.exec(syncTask) != QDialog::Accepted) {
        qDebug() << "[SVLConnectPage] Mod sync task cancelled or aborted.";
    }
}
