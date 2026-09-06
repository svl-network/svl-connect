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
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QMessageBox>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
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

    m_addServerBtn = new QPushButton(tr("➕ ADD SERVER"), centralContainer);
    m_addServerBtn->setObjectName("addCustomServerButton");
    m_addServerBtn->setCursor(Qt::PointingHandCursor);
    m_addServerBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; border: none; border-radius: 8px; padding: 10px 18px; font-weight: 800; font-size: 12px; } QPushButton:hover { background-color: #10FFAC; } QPushButton:pressed { background-color: #00B377; }");
    connect(m_addServerBtn, &QPushButton::clicked, this, &SVLConnectPage::openAddCustomServerDialog);
    searchRow->addWidget(m_addServerBtn);

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

    // 4. Partner & Store Promotion Bar
    auto* partnerFrame = new QFrame(centralContainer);
    partnerFrame->setObjectName("partnerPromotionFrame");
    partnerFrame->setStyleSheet("QFrame#partnerPromotionFrame { background-color: #14171E; border: 1px solid #232A36; border-radius: 8px; padding: 10px 14px; }");
    auto* partnerLayout = new QHBoxLayout(partnerFrame);
    partnerLayout->setContentsMargins(4, 2, 4, 2);
    partnerLayout->setSpacing(12);

    auto* partnerTextLayout = new QVBoxLayout();
    partnerTextLayout->setSpacing(2);
    auto* partnerTitle = new QLabel(tr("OFFICIAL PARTNER: BISECTHOSTING"), partnerFrame);
    partnerTitle->setStyleSheet("color: #38BDF8; font-size: 10px; font-weight: 700; letter-spacing: 0.5px; background: transparent;");
    auto* partnerDesc = new QLabel(tr("Rent a high-speed Minecraft server with automated SVL-Bridge support. Save 25% with code SUNVEIL."), partnerFrame);
    partnerDesc->setStyleSheet("color: #94A3B8; font-size: 11px; background: transparent;");
    partnerTextLayout->addWidget(partnerTitle);
    partnerTextLayout->addWidget(partnerDesc);
    partnerLayout->addLayout(partnerTextLayout, 1);

    auto* btnBisect = new QPushButton(tr("25% Off Hosting ↗"), partnerFrame);
    btnBisect->setCursor(Qt::PointingHandCursor);
    btnBisect->setStyleSheet("QPushButton { background-color: #0284C7; color: #FFFFFF; border: none; border-radius: 6px; padding: 6px 12px; font-size: 11px; font-weight: 700; } QPushButton:hover { background-color: #0369A1; }");
    connect(btnBisect, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://www.bisecthosting.com/clients/aff.php?aff=7448"));
    });
    partnerLayout->addWidget(btnBisect);

    auto* btnStore = new QPushButton(tr("Store ↗"), partnerFrame);
    btnStore->setCursor(Qt::PointingHandCursor);
    btnStore->setStyleSheet("QPushButton { background-color: #10B981; color: #061B14; border: none; border-radius: 6px; padding: 6px 12px; font-size: 11px; font-weight: 700; } QPushButton:hover { background-color: #059669; }");
    connect(btnStore, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://sunveilsmp.tebex.io"));
    });
    partnerLayout->addWidget(btnStore);

    auto* btnKofi = new QPushButton(tr("Tip Jar ↗"), partnerFrame);
    btnKofi->setCursor(Qt::PointingHandCursor);
    btnKofi->setStyleSheet("QPushButton { background-color: #242B35; color: #F43F5E; border: 1px solid #364152; border-radius: 6px; padding: 6px 12px; font-size: 11px; font-weight: 700; } QPushButton:hover { background-color: #2F3846; }");
    connect(btnKofi, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://ko-fi.com/X8X815R6AF"));
    });
    partnerLayout->addWidget(btnKofi);

    mainLayout->addWidget(partnerFrame);

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
                    model.isOnline = obj.contains("online") ? obj.value("online").toBool(false) : (statObj.contains("online") ? statObj.value("online").toBool(false) : false);

                    model.boosts = obj.value("boosts").toInt(0);
                    model.sponsored = obj.value("sponsored").toBool(false);
                    model.bannerUrl = obj.value("bannerUrl").toString();

                    if (obj.contains("tunnel") && obj.value("tunnel").isObject()) {
                        model.isTunnel = obj.value("tunnel").toObject().value("active").toBool(false);
                    }

                    QJsonObject linksObj = obj.value("links").toObject();
                    model.links.store = linksObj.value("store").toString();
                    model.links.discord = linksObj.value("discord").toString();
                    model.links.website = linksObj.value("website").toString();

                    m_allServers.append(model);
                }
            }
        }
    }

    // Load and prepend user's local custom standalone servers
    loadCustomServers();
    for (int i = m_customServers.size() - 1; i >= 0; --i) {
        m_allServers.prepend(m_customServers[i]);
    }

    onSearchFilterChanged(m_currentQuery);
}

void SVLConnectPage::loadCustomServers()
{
    m_customServers.clear();
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString filePath = QDir(dataDir).filePath("custom_servers.json");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray fileBytes = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(fileBytes);
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const auto& val : arr) {
            if (val.isObject()) {
                QJsonObject obj = val.toObject();
                SVLServerModel model;
                model.serverKey = obj.value("serverKey").toString();
                if (model.serverKey.isEmpty()) continue;
                model.name = obj.value("name").toString("Custom Server");
                model.ip = obj.value("ip").toString("127.0.0.1");
                model.port = static_cast<quint16>(obj.value("port").toInt(25565));
                model.mcVersion = obj.value("mcVersion").toString("1.21.1");
                model.loader = obj.value("loader").toString("vanilla");
                model.loaderVersion = obj.value("loaderVersion").toString();
                model.motd = obj.value("motd").toString("Direct custom connection");
                model.icon = obj.value("icon").toString();
                model.verified = false;
                model.isOnline = true;
                model.isCustom = true;
                model.isTunnel = false;
                model.players = 0;
                model.maxPlayers = 50;
                model.modCount = 0;
                m_customServers.append(model);
            }
        }
    }
}

void SVLConnectPage::saveCustomServers()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString filePath = QDir(dataDir).filePath("custom_servers.json");

    QJsonArray arr;
    for (const auto& srv : m_customServers) {
        QJsonObject obj;
        obj["serverKey"] = srv.serverKey;
        obj["name"] = srv.name;
        obj["ip"] = srv.ip;
        obj["port"] = srv.port;
        obj["mcVersion"] = srv.mcVersion;
        obj["loader"] = srv.loader;
        obj["loaderVersion"] = srv.loaderVersion;
        obj["motd"] = srv.motd;
        obj["icon"] = srv.icon;
        arr.append(obj);
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson());
    }
}

void SVLConnectPage::deleteCustomServer(const QString& serverKey)
{
    for (int i = 0; i < m_customServers.size(); ++i) {
        if (m_customServers[i].serverKey == serverKey) {
            m_customServers.removeAt(i);
            break;
        }
    }
    saveCustomServers();
    refreshServers();
}

void SVLConnectPage::openAddCustomServerDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Add Custom Minecraft Server"));
    dialog.setMinimumWidth(480);
    dialog.setStyleSheet("QDialog { background-color: #18181B; color: #FFFFFF; } "
                         "QLabel { color: #E4E4E7; font-size: 13px; font-weight: 600; } "
                         "QLineEdit, QSpinBox, QComboBox { background-color: #27272A; border: 1px solid #3F3F46; border-radius: 6px; padding: 8px 12px; color: #FFFFFF; font-size: 13px; } "
                         "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #00E599; }");

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* headerTitle = new QLabel(tr("➕ Add Custom Server"), &dialog);
    headerTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #FFFFFF;");
    layout->addWidget(headerTitle);

    auto* headerSub = new QLabel(tr("Add any vanilla, modded, or standalone Minecraft server directly to your launcher."), &dialog);
    headerSub->setStyleSheet("font-size: 12px; color: #A1A1AA; font-weight: 400;");
    headerSub->setWordWrap(true);
    layout->addWidget(headerSub);

    auto* formLayout = new QVBoxLayout();
    formLayout->setSpacing(10);

    auto* nameLabel = new QLabel(tr("Server Name *"), &dialog);
    auto* nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText(tr("e.g. Hypixel Network or Local Survival"));
    formLayout->addWidget(nameLabel);
    formLayout->addWidget(nameEdit);

    auto* hostRow = new QHBoxLayout();
    hostRow->setSpacing(12);

    auto* ipCol = new QVBoxLayout();
    auto* ipLabel = new QLabel(tr("Host / IP Address *"), &dialog);
    auto* ipEdit = new QLineEdit(&dialog);
    ipEdit->setPlaceholderText(tr("e.g. mc.hypixel.net or 127.0.0.1"));
    ipCol->addWidget(ipLabel);
    ipCol->addWidget(ipEdit);
    hostRow->addLayout(ipCol, 3);

    auto* portCol = new QVBoxLayout();
    auto* portLabel = new QLabel(tr("Port"), &dialog);
    auto* portSpin = new QSpinBox(&dialog);
    portSpin->setRange(1, 65535);
    portSpin->setValue(25565);
    portCol->addWidget(portLabel);
    portCol->addWidget(portSpin);
    hostRow->addLayout(portCol, 1);

    formLayout->addLayout(hostRow);

    auto* versionRow = new QHBoxLayout();
    versionRow->setSpacing(12);

    auto* verCol = new QVBoxLayout();
    auto* verLabel = new QLabel(tr("Minecraft Version"), &dialog);
    auto* verCombo = new QComboBox(&dialog);
    verCombo->setEditable(true);
    verCombo->addItems({"1.21.1", "1.21", "1.20.6", "1.20.4", "1.20.1", "1.19.4", "1.18.2", "1.16.5", "1.12.2", "1.8.9"});
    verCol->addWidget(verLabel);
    verCol->addWidget(verCombo);
    versionRow->addLayout(verCol, 1);

    auto* loaderCol = new QVBoxLayout();
    auto* loaderLabel = new QLabel(tr("Loader / Flavor"), &dialog);
    auto* loaderCombo = new QComboBox(&dialog);
    loaderCombo->addItems({"vanilla", "paper", "fabric", "forge", "neoforge"});
    loaderCol->addWidget(loaderLabel);
    loaderCol->addWidget(loaderCombo);
    versionRow->addLayout(loaderCol, 1);

    formLayout->addLayout(versionRow);

    auto* motdLabel = new QLabel(tr("MOTD / Description (Optional)"), &dialog);
    auto* motdEdit = new QLineEdit(&dialog);
    motdEdit->setPlaceholderText(tr("Custom multiplayer server"));
    formLayout->addWidget(motdLabel);
    formLayout->addWidget(motdEdit);

    layout->addLayout(formLayout);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    auto* cancelBtn = new QPushButton(tr("Cancel"), &dialog);
    cancelBtn->setStyleSheet("QPushButton { background-color: #27272A; color: #FFFFFF; border: 1px solid #3F3F46; border-radius: 6px; padding: 10px 20px; font-weight: 600; font-size: 13px; } QPushButton:hover { background-color: #3F3F46; }");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto* saveBtn = new QPushButton(tr("Add Server"), &dialog);
    saveBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; border: none; border-radius: 6px; padding: 10px 24px; font-weight: 800; font-size: 13px; } QPushButton:hover { background-color: #10FFAC; }");
    connect(saveBtn, &QPushButton::clicked, [&]() {
        QString name = nameEdit->text().trimmed();
        QString ip = ipEdit->text().trimmed();
        if (name.isEmpty() || ip.isEmpty()) {
            QMessageBox::warning(&dialog, tr("Missing Fields"), tr("Please enter both a server name and a server address."));
            return;
        }

        SVLServerModel model;
        model.serverKey = "custom_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        model.name = name;
        model.ip = ip;
        model.port = static_cast<quint16>(portSpin->value());
        model.mcVersion = verCombo->currentText().trimmed();
        if (model.mcVersion.isEmpty()) model.mcVersion = "1.21.1";
        model.loader = loaderCombo->currentText().trimmed().toLower();
        model.motd = motdEdit->text().trimmed();
        if (model.motd.isEmpty()) model.motd = tr("Direct custom standalone connection");
        model.verified = false;
        model.isOnline = true;
        model.isCustom = true;
        model.isTunnel = false;
        model.players = 0;
        model.maxPlayers = 50;
        model.modCount = 0;

        m_customServers.prepend(model);
        saveCustomServers();
        refreshServers();
        dialog.accept();
    });
    btnRow->addWidget(saveBtn);

    layout->addLayout(btnRow);
    dialog.exec();
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
        auto* emptyWidget = new QWidget();
        auto* emptyLayout = new QVBoxLayout(emptyWidget);
        emptyLayout->setContentsMargins(40, 60, 40, 60);
        emptyLayout->setSpacing(12);
        emptyLayout->setAlignment(Qt::AlignCenter);

        auto* iconLabel = new QLabel(tr("🔍"), emptyWidget);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("font-size: 48px; background: transparent;");
        emptyLayout->addWidget(iconLabel);

        QString titleText = m_currentQuery.isEmpty() ? tr("No Minecraft servers found") : tr("No realms matching \"%1\"").arg(m_currentQuery);
        auto* noMatchesLabel = new QLabel(titleText, emptyWidget);
        noMatchesLabel->setAlignment(Qt::AlignCenter);
        noMatchesLabel->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: 700; background: transparent;");
        emptyLayout->addWidget(noMatchesLabel);

        QString subText = m_currentQuery.isEmpty()
                              ? tr("No live Bridge realms online yet. Click '➕ ADD SERVER' above to add your custom server!")
                              : tr("Check your search terms, add a custom server above, or refresh the directory.");
        auto* subTextLabel = new QLabel(subText, emptyWidget);
        subTextLabel->setAlignment(Qt::AlignCenter);
        subTextLabel->setStyleSheet("color: #71717A; font-size: 13px; font-weight: 400; background: transparent;");
        emptyLayout->addWidget(subTextLabel);

        m_cardsLayout->addWidget(emptyWidget);
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

    if (server.isCustom) {
        auto* customBadge = new QLabel(tr("🌐 CUSTOM SERVER"), card);
        customBadge->setObjectName("badgeCustom");
        customBadge->setStyleSheet("background-color: rgba(6, 182, 212, 0.15); color: #06B6D4; border: 1px solid rgba(6, 182, 212, 0.4); border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 800;");
        titleRow->addWidget(customBadge);
    }

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

    if (server.isTunnel) {
        auto* tunnelLabel = new QLabel(tr("🛡️ SVL SHIELD"), card);
        tunnelLabel->setObjectName("badgeMeta");
        tunnelLabel->setStyleSheet("background-color: #0F2E1E; color: #00E599; border: 1px solid #10B981; border-radius: 6px; padding: 3px 8px; font-size: 11px; font-weight: 700;");
        tunnelLabel->setToolTip(tr("Protected by Sunveil Secure Tunnel: Zero-portforwarding & 100% masked IP."));
        badgesLayout->addWidget(tunnelLabel);
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
    playersLabel->setVisible(server.isOnline && !server.isCustom);
    rightLayout->addWidget(playersLabel, 0, Qt::AlignRight);

    auto* actionButtonsLayout = new QHBoxLayout();
    actionButtonsLayout->setSpacing(8);

    if (server.isCustom) {
        auto* removeBtn = new QPushButton(tr("Remove"), card);
        removeBtn->setCursor(Qt::PointingHandCursor);
        removeBtn->setStyleSheet("QPushButton { background-color: #27272A; color: #F87171; border: 1px solid #3F3F46; border-radius: 8px; padding: 8px 14px; font-weight: 600; font-size: 12px; min-height: 20px; } QPushButton:hover { background-color: rgba(239, 68, 68, 0.2); border-color: #EF4444; }");
        connect(removeBtn, &QPushButton::clicked, this, [this, serverKey = server.serverKey]() {
            deleteCustomServer(serverKey);
        });
        actionButtonsLayout->addWidget(removeBtn);
    } else {
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
    }

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
    auto* syncTask = new SVLModSyncTask(m_masterApiBaseUrl, server.serverKey, server.name, server.ip, server.port, server.mcVersion, server.loader, this);

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
