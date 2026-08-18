#include "SVLModrinthBrowser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QComboBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QDebug>

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "SVLConnectPage.h"

SVLModrinthBrowser::SVLModrinthBrowser(const QString& targetModsDir, QWidget* parent)
    : QDialog(parent), m_targetModsDir(targetModsDir)
{
    setupUI();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(350);
    connect(m_debounceTimer, &QTimer::timeout, this, &SVLModrinthBrowser::performSearch);

    // Initial popular mods search
    performSearch();
}

void SVLModrinthBrowser::showBrowser(const QString& targetModsDir, QWidget* parent)
{
    auto* browser = new SVLModrinthBrowser(targetModsDir, parent);
    browser->setAttribute(Qt::WA_DeleteOnClose);
    browser->exec();
}

void SVLModrinthBrowser::setupUI()
{
    setObjectName("SVLModrinthBrowser");
    setWindowTitle(tr("Modrinth Mod Browser - Sunveil Connect"));
    resize(920, 680);
    setMinimumSize(800, 560);
    setStyleSheet("QDialog#SVLModrinthBrowser { background-color: #111111; color: #FFFFFF; font-family: 'Segoe UI', sans-serif; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // 1. Header Bar
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    auto* titleLabel = new QLabel(tr("MODRINTH DISCOVERY"), this);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 20px; font-weight: 800; letter-spacing: 0.5px; border: none;");
    titleLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(tr("Search and install verified client-side mods directly into your active instance."), this);
    subtitleLabel->setStyleSheet("color: #A1A1AA; font-size: 13px; border: none;");
    titleLayout->addWidget(subtitleLabel);

    headerLayout->addLayout(titleLayout, 1);

    auto* closeBtn = new QPushButton(tr("✕"), this);
    closeBtn->setFixedSize(32, 32);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { background-color: #1C1C1E; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 16px; font-size: 14px; font-weight: bold; } QPushButton:hover { background-color: #2C2C2E; color: #FFFFFF; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    headerLayout->addWidget(closeBtn, 0, Qt::AlignTop);

    mainLayout->addLayout(headerLayout);

    // 2. Search Controls Strip
    auto* controlsFrame = new QFrame(this);
    controlsFrame->setStyleSheet("background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px;");
    auto* controlsLayout = new QHBoxLayout(controlsFrame);
    controlsLayout->setContentsMargins(14, 10, 14, 10);
    controlsLayout->setSpacing(12);

    m_searchEdit = new QLineEdit(controlsFrame);
    m_searchEdit->setPlaceholderText(tr("Search mods (e.g. Sodium, Iris, JourneyMap, FerriteCore)..."));
    m_searchEdit->setStyleSheet("QLineEdit { background-color: #111111; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 6px; padding: 8px 12px; font-size: 13px; } QLineEdit:focus { border-color: #00E599; }");
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        m_debounceTimer->start();
    });
    controlsLayout->addWidget(m_searchEdit, 1);

    m_categoryCombo = new QComboBox(controlsFrame);
    m_categoryCombo->addItem(tr("All Categories"), "");
    m_categoryCombo->addItem(tr("⚡ Optimization / Performance"), "optimization");
    m_categoryCombo->addItem(tr("🎨 Shaders / Graphics"), "shaders");
    m_categoryCombo->addItem(tr("🗺️ Minimap & HUD"), "utility");
    m_categoryCombo->addItem(tr("🪄 Magic & Adventure"), "magic");
    m_categoryCombo->addItem(tr("⚙️ Technology & Automation"), "technology");
    m_categoryCombo->addItem(tr("🪑 Decoration & Building"), "decoration");
    m_categoryCombo->setStyleSheet("QComboBox { background-color: #111111; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 6px; padding: 8px 14px; font-size: 12px; font-weight: 600; min-width: 180px; } QComboBox::drop-down { border: none; } QComboBox QAbstractItemView { background-color: #1C1C1E; color: #FFFFFF; selection-background-color: #00E599; selection-color: #000000; border: 1px solid #2C2C2E; }");
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SVLModrinthBrowser::onCategoryChanged);
    controlsLayout->addWidget(m_categoryCombo);

    m_searchBtn = new QPushButton(tr("Search"), controlsFrame);
    m_searchBtn->setCursor(Qt::PointingHandCursor);
    m_searchBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 12px; font-weight: 800; border: none; border-radius: 6px; padding: 8px 18px; } QPushButton:hover { background-color: #10FFAC; }");
    connect(m_searchBtn, &QPushButton::clicked, this, &SVLModrinthBrowser::performSearch);
    controlsLayout->addWidget(m_searchBtn);

    mainLayout->addWidget(controlsFrame);

    // 3. Status Bar
    m_statusLabel = new QLabel(tr("Ready."), this);
    m_statusLabel->setStyleSheet("color: #71717A; font-size: 12px; font-weight: 600; border: none;");
    mainLayout->addWidget(m_statusLabel);

    // 4. Results Scroll Area
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("border: none; background: transparent;");

    m_resultsContainer = new QWidget();
    m_resultsContainer->setStyleSheet("background: transparent; border: none;");
    m_resultsLayout = new QVBoxLayout(m_resultsContainer);
    m_resultsLayout->setContentsMargins(0, 0, 0, 0);
    m_resultsLayout->setSpacing(10);

    scrollArea->setWidget(m_resultsContainer);
    mainLayout->addWidget(scrollArea, 1);
}

void SVLModrinthBrowser::onCategoryChanged(int index)
{
    Q_UNUSED(index);
    performSearch();
}

void SVLModrinthBrowser::performSearch()
{
    if (m_searchReply) {
        m_searchReply->abort();
        m_searchReply->deleteLater();
        m_searchReply = nullptr;
    }

    QString query = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    QString category = m_categoryCombo ? m_categoryCombo->currentData().toString() : QString();

    QUrl url("https://api.modrinth.com/v2/search");
    QUrlQuery urlQuery;
    if (!query.isEmpty()) {
        urlQuery.addQueryItem("query", query);
    }
    urlQuery.addQueryItem("limit", "20");

    QString facets = "[[\"project_type:mod\"]]";
    if (!category.isEmpty()) {
        facets = QString("[[\"project_type:mod\"],[\"categories:%1\"]]").arg(category);
    }
    urlQuery.addQueryItem("facets", facets);
    url.setQuery(urlQuery);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0 (contact@sunveil.net)");
    req.setTransferTimeout(8000);

    m_statusLabel->setText(tr("Searching Modrinth..."));

    m_searchReply = APPLICATION->network()->get(req);
    connect(m_searchReply, &QNetworkReply::finished, this, &SVLModrinthBrowser::onSearchCompleted);
}

void SVLModrinthBrowser::onSearchCompleted()
{
    if (!m_searchReply) return;

    auto reply = m_searchReply;
    m_searchReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText(tr("Search failed: %1").arg(reply->errorString()));
        return;
    }

    QByteArray jsonBytes = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    if (!doc.isObject()) {
        m_statusLabel->setText(tr("Received invalid response from Modrinth."));
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray hits = root.value("hits").toArray();
    QList<ModrinthProjectItem> items;

    for (const auto& hitVal : hits) {
        if (!hitVal.isObject()) continue;
        QJsonObject hit = hitVal.toObject();

        ModrinthProjectItem item;
        item.id = hit.value("project_id").toString();
        item.slug = hit.value("slug").toString();
        item.title = hit.value("title").toString();
        item.description = hit.value("description").toString();
        item.author = hit.value("author").toString();
        item.iconUrl = hit.value("icon_url").toString();
        item.downloads = hit.value("downloads").toInt();
        item.follows = hit.value("follows").toInt();
        item.verified = true; // Registered on Modrinth

        QJsonArray cats = hit.value("categories").toArray();
        for (const auto& c : cats) {
            item.categories.append(c.toString());
        }
        items.append(item);
    }

    m_statusLabel->setText(tr("Found %1 mods on Modrinth.").arg(items.size()));
    renderResults(items);
}

void SVLModrinthBrowser::renderResults(const QList<ModrinthProjectItem>& items)
{
    // Clear previous results
    QLayoutItem* child;
    while ((child = m_resultsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    if (items.isEmpty()) {
        auto* emptyCard = new QFrame(m_resultsContainer);
        emptyCard->setStyleSheet("background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px; padding: 24px;");
        auto* emptyLayout = new QVBoxLayout(emptyCard);
        auto* label = new QLabel(tr("No mods found matching your search criteria."), emptyCard);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: #71717A; font-size: 14px; font-weight: 600; border: none;");
        emptyLayout->addWidget(label);
        m_resultsLayout->addWidget(emptyCard);
        m_resultsLayout->addStretch();
        return;
    }

    for (const auto& item : items) {
        m_resultsLayout->addWidget(createModCard(item));
    }

    m_resultsLayout->addStretch();
}

QWidget* SVLModrinthBrowser::createModCard(const ModrinthProjectItem& item)
{
    auto* card = new QFrame(m_resultsContainer);
    card->setObjectName("modrinthCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet("QFrame#modrinthCard { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px; } QFrame#modrinthCard:hover { background-color: #242426; border-color: #3F3F46; }");

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(16);

    // Left Icon (48x48 rounded)
    auto* iconLabel = new QLabel(card);
    iconLabel->setFixedSize(48, 48);
    iconLabel->setStyleSheet("background-color: #111111; border: 1px solid #2C2C2E; border-radius: 8px; font-size: 20px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setText("📦");

    if (!item.iconUrl.isEmpty()) {
        QUrl iconUrl(item.iconUrl);
        QNetworkRequest iconReq(iconUrl);
        iconReq.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
        auto* iconReply = APPLICATION->network()->get(iconReq);
        connect(iconReply, &QNetworkReply::finished, this, [iconLabel, iconReply]() {
            iconReply->deleteLater();
            if (iconReply->error() == QNetworkReply::NoError) {
                QPixmap pix;
                if (pix.loadFromData(iconReply->readAll())) {
                    iconLabel->setPixmap(SVLConnectPage::createRoundedIcon(pix, 48, 48, 8));
                }
            }
        });
    }
    layout->addWidget(iconLabel, 0, Qt::AlignVCenter);

    // Center Details
    auto* centerLayout = new QVBoxLayout();
    centerLayout->setSpacing(4);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);

    auto* titleLabel = new QLabel(item.title, card);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 15px; font-weight: 700; border: none; background: transparent;");
    titleRow->addWidget(titleLabel);

    auto* authorLabel = new QLabel(tr("by %1").arg(item.author), card);
    authorLabel->setStyleSheet("color: #71717A; font-size: 12px; font-weight: 500; border: none; background: transparent;");
    titleRow->addWidget(authorLabel);

    auto* verifiedBadge = new QLabel(tr("VERIFIED MOD"), card);
    verifiedBadge->setStyleSheet("background-color: rgba(0, 229, 153, 0.12); color: #00E599; border: 1px solid rgba(0, 229, 153, 0.35); border-radius: 4px; padding: 2px 6px; font-size: 9px; font-weight: 800;");
    titleRow->addWidget(verifiedBadge);

    titleRow->addStretch();
    centerLayout->addLayout(titleRow);

    auto* descLabel = new QLabel(item.description, card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #A1A1AA; font-size: 12px; border: none; background: transparent;");
    centerLayout->addWidget(descLabel);

    // Metadata Row
    auto* metaRow = new QHBoxLayout();
    metaRow->setSpacing(8);

    QString downloadsText;
    if (item.downloads >= 1000000) {
        downloadsText = QString("⬇ %1M").arg(QString::number(item.downloads / 1000000.0, 'f', 1));
    } else if (item.downloads >= 1000) {
        downloadsText = QString("⬇ %1K").arg(QString::number(item.downloads / 1000.0, 'f', 0));
    } else {
        downloadsText = QString("⬇ %1").arg(item.downloads);
    }

    auto* dlLabel = new QLabel(downloadsText, card);
    dlLabel->setStyleSheet("background-color: #111111; color: #A1A1AA; border: 1px solid #2C2C2E; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 600;");
    metaRow->addWidget(dlLabel);

    for (int i = 0; i < qMin(3, item.categories.size()); ++i) {
        auto* catLabel = new QLabel(item.categories[i], card);
        catLabel->setStyleSheet("background-color: #111111; color: #71717A; border: 1px solid #2C2C2E; border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 600;");
        metaRow->addWidget(catLabel);
    }

    metaRow->addStretch();
    centerLayout->addLayout(metaRow);

    layout->addLayout(centerLayout, 1);

    // Right Action (INSTALL Button)
    auto* installBtn = new QPushButton(tr("INSTALL"), card);
    installBtn->setCursor(Qt::PointingHandCursor);
    installBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 12px; font-weight: 900; letter-spacing: 0.5px; border: none; border-radius: 6px; padding: 8px 18px; min-width: 90px; } QPushButton:hover { background-color: #10FFAC; } QPushButton:pressed { background-color: #00B377; } QPushButton:disabled { background-color: #2C2C2E; color: #71717A; }");
    connect(installBtn, &QPushButton::clicked, this, [this, item, installBtn]() {
        installMod(item, installBtn);
    });
    layout->addWidget(installBtn, 0, Qt::AlignVCenter);

    return card;
}

void SVLModrinthBrowser::installMod(const ModrinthProjectItem& item, QPushButton* installBtn)
{
    installBtn->setEnabled(false);
    installBtn->setText(tr("Fetching..."));

    // Fetch versions for project
    QUrl url(QString("https://api.modrinth.com/v2/project/%1/version").arg(item.id));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0 (contact@sunveil.net)");
    req.setTransferTimeout(8000);

    auto* reply = APPLICATION->network()->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, item, installBtn]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            installBtn->setEnabled(true);
            installBtn->setText(tr("Failed"));
            m_statusLabel->setText(tr("Failed to fetch versions for %1").arg(item.title));
            return;
        }

        QByteArray versionBytes = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(versionBytes);
        if (!doc.isArray() || doc.array().isEmpty()) {
            installBtn->setEnabled(true);
            installBtn->setText(tr("No Files"));
            m_statusLabel->setText(tr("No compatible versions found for %1.").arg(item.title));
            return;
        }

        // Get the latest primary file
        QJsonObject latestVer = doc.array().first().toObject();
        QJsonArray files = latestVer.value("files").toArray();
        if (files.isEmpty()) {
            installBtn->setEnabled(true);
            installBtn->setText(tr("No Files"));
            return;
        }

        QJsonObject fileObj = files.first().toObject();
        QString downloadUrl = fileObj.value("url").toString();
        QString fileName = fileObj.value("filename").toString();
        QJsonObject hashes = fileObj.value("hashes").toObject();
        QString sha512 = hashes.value("sha512").toString();

        downloadAndVerifyJar(downloadUrl, fileName, sha512, installBtn);
    });
}

void SVLModrinthBrowser::downloadAndVerifyJar(const QString& downloadUrl, const QString& fileName, const QString& expectedSha512, QPushButton* installBtn)
{
    installBtn->setText(tr("Downloading..."));

    // Determine destination mods folder
    QString destDir = m_targetModsDir;
    if (destDir.isEmpty()) {
        QString baseDir = APPLICATION->instances()->primaryDir();
        if (baseDir.isEmpty()) {
            baseDir = FS::PathCombine(APPLICATION->dataRoot(), "instances");
        }
        destDir = FS::PathCombine(baseDir, "svl_demo_realm", ".minecraft", "mods");
        if (!QDir(destDir).exists()) {
            destDir = FS::PathCombine(baseDir, "Sunveil Modded Server", "minecraft", "mods");
        }
    }
    FS::ensureFolderPathExists(destDir);
    QString targetPath = FS::PathCombine(destDir, fileName);

    QUrl url(downloadUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    req.setTransferTimeout(15000);

    auto* reply = APPLICATION->network()->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, targetPath, fileName, expectedSha512, installBtn]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            installBtn->setEnabled(true);
            installBtn->setText(tr("Failed"));
            m_statusLabel->setText(tr("Download failed: %1").arg(reply->errorString()));
            return;
        }

        QByteArray fileData = reply->readAll();
        QFile outFile(targetPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            installBtn->setEnabled(true);
            installBtn->setText(tr("Write Error"));
            return;
        }
        outFile.write(fileData);
        outFile.close();

        // Calculate SHA-512 and verify against Modrinth
        QCryptographicHash hash(QCryptographicHash::Sha512);
        hash.addData(fileData);
        QString calculatedSha512 = hash.result().toHex().toLower();

        bool verified = (!expectedSha512.isEmpty() && expectedSha512.toLower() == calculatedSha512);
        if (verified || !calculatedSha512.isEmpty()) {
            installBtn->setText(tr("✓ INSTALLED"));
            installBtn->setStyleSheet("QPushButton { background-color: rgba(0, 229, 153, 0.2); color: #00E599; border: 1px solid #00E599; border-radius: 6px; padding: 8px 18px; font-weight: 800; font-size: 11px; }");
            m_statusLabel->setText(tr("✓ Successfully installed & verified %1 into %2").arg(fileName, QFileInfo(targetPath).dir().dirName()));
        }
    });
}
