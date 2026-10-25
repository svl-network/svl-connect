#include "SVLMultiplayerHostDialog.h"
#include "ui/pages/svl/SVLHostingManager.h"
#include "minecraft/MinecraftInstance.h"
#include "Application.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QProgressBar>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFrame>

SVLMultiplayerHostDialog::SVLMultiplayerHostDialog(MinecraftInstance* instance, SVLHostingManager* hostingManager, QWidget* parent)
    : QDialog(parent), m_instance(instance), m_hostingManager(hostingManager) {
    setWindowTitle(tr("Host Multiplayer World - Sunveil Network"));
    setFixedSize(540, 500);
    setStyleSheet("QDialog { background-color: #121217; color: #ffffff; }");

    setupUI();

    if (m_hostingManager) {
        connect(m_hostingManager, &SVLHostingManager::p2pShareCodeReady, this, &SVLMultiplayerHostDialog::onP2PShareCodeReady);
        connect(m_hostingManager, &SVLHostingManager::cloudServerCreated, this, &SVLMultiplayerHostDialog::onCloudServerCreated);
        connect(m_hostingManager, &SVLHostingManager::hostingError, this, &SVLMultiplayerHostDialog::onHostingError);
    }
}

void SVLMultiplayerHostDialog::showHostDialog(MinecraftInstance* instance, SVLHostingManager* hostingManager, QWidget* parent) {
    if (!instance) {
        QMessageBox::warning(parent, tr("No Instance"), tr("Please select a valid Minecraft instance first."));
        return;
    }
    auto* dlg = new SVLMultiplayerHostDialog(instance, hostingManager, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
}

void SVLMultiplayerHostDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto* headerTitle = new QLabel(tr("Host World to Friends"), this);
    headerTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #ffffff;");
    auto* headerDesc = new QLabel(tr("Instance: %1").arg(m_instance ? m_instance->name() : "Unknown"), this);
    headerDesc->setStyleSheet("color: #9ca3af; font-size: 13px;");

    mainLayout->addWidget(headerTitle);
    mainLayout->addWidget(headerDesc);

    // Mode Selector Cards
    auto* optionsGroup = new QButtonGroup(this);

    m_radioP2P = new QRadioButton(tr("Option A: Peer-to-Peer Tunnel (Free, up to 8 Players)"), this);
    m_radioP2P->setChecked(true);
    m_radioP2P->setStyleSheet("QRadioButton { font-weight: bold; font-size: 14px; color: #38bdf8; }");

    auto* p2pDesc = new QLabel(tr("Routes your singleplayer LAN world securely through the Sunveil Tunnel Relay. Friends join via a share code without port-forwarding."), this);
    p2pDesc->setWordWrap(true);
    p2pDesc->setStyleSheet("color: #94a3b8; font-size: 12px; margin-left: 24px;");

    m_radioCloud = new QRadioButton(tr("Option B: One-Click Cloud Server Creation"), this);
    m_radioCloud->setStyleSheet("QRadioButton { font-weight: bold; font-size: 14px; color: #a855f7; margin-top: 8px; }");

    auto* cloudDesc = new QLabel(tr("Spins up a 24/7 dedicated cloud realm instance directly linked to your Sunveil account slot with automatic bridge plugin setup."), this);
    cloudDesc->setWordWrap(true);
    cloudDesc->setStyleSheet("color: #94a3b8; font-size: 12px; margin-left: 24px;");

    optionsGroup->addButton(m_radioP2P);
    optionsGroup->addButton(m_radioCloud);

    mainLayout->addWidget(m_radioP2P);
    mainLayout->addWidget(p2pDesc);
    mainLayout->addWidget(m_radioCloud);
    mainLayout->addWidget(cloudDesc);

    connect(m_radioP2P, &QRadioButton::toggled, this, &SVLMultiplayerHostDialog::onModeToggled);

    // Separator line
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #272730;");
    mainLayout->addWidget(line);

    // Form settings
    auto* formLayout = new QVBoxLayout();

    auto* worldLabel = new QLabel(tr("Session or World Name:"), this);
    worldLabel->setStyleSheet("color: #d1d5db; font-size: 12px;");
    m_worldNameEdit = new QLineEdit(this);
    m_worldNameEdit->setText(m_instance ? m_instance->name() : "My World");
    m_worldNameEdit->setStyleSheet("QLineEdit { background: #1a1a24; border: 1px solid #333342; border-radius: 6px; padding: 8px; color: #fff; }");

    auto* playersLabel = new QLabel(tr("Max Players (P2P limit: 8):"), this);
    playersLabel->setStyleSheet("color: #d1d5db; font-size: 12px; margin-top: 6px;");
    m_maxPlayersSpin = new QSpinBox(this);
    m_maxPlayersSpin->setRange(2, 8);
    m_maxPlayersSpin->setValue(8);
    m_maxPlayersSpin->setStyleSheet("QSpinBox { background: #1a1a24; border: 1px solid #333342; border-radius: 6px; padding: 6px; color: #fff; }");

    m_serverSubdomainEdit = new QLineEdit(this);
    m_serverSubdomainEdit->setPlaceholderText(tr("Requested subdomain (e.g. dragon-smp)"));
    m_serverSubdomainEdit->setStyleSheet("QLineEdit { background: #1a1a24; border: 1px solid #333342; border-radius: 6px; padding: 8px; color: #fff; }");
    m_serverSubdomainEdit->setVisible(false);

    formLayout->addWidget(worldLabel);
    formLayout->addWidget(m_worldNameEdit);
    formLayout->addWidget(playersLabel);
    formLayout->addWidget(m_maxPlayersSpin);
    formLayout->addWidget(m_serverSubdomainEdit);
    mainLayout->addLayout(formLayout);

    // Status Banner / Share Code Display
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #10b981; font-weight: bold; font-size: 13px;");
    m_statusLabel->setVisible(false);

    m_shareCodeLabel = new QLabel(this);
    m_shareCodeLabel->setStyleSheet("background: #1e1b4b; color: #a5b4fc; border: 1px dashed #6366f1; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 15px;");
    m_shareCodeLabel->setAlignment(Qt::AlignCenter);
    m_shareCodeLabel->setVisible(false);

    m_copyInviteBtn = new QPushButton(tr("📋 Copy Invite Link"), this);
    m_copyInviteBtn->setStyleSheet("QPushButton { background: #4338ca; color: white; border-radius: 6px; padding: 6px 12px; font-weight: bold; } QPushButton:hover { background: #3730a3; }");
    m_copyInviteBtn->setVisible(false);
    connect(m_copyInviteBtn, &QPushButton::clicked, this, &SVLMultiplayerHostDialog::copyInviteToClipboard);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0); // indeterminate
    m_progressBar->setVisible(false);

    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_shareCodeLabel);
    mainLayout->addWidget(m_copyInviteBtn);
    mainLayout->addWidget(m_progressBar);

    mainLayout->addStretch();

    // Dialog Button Bar
    auto* btnLayout = new QHBoxLayout();
    m_cancelBtn = new QPushButton(tr("Close"), this);
    m_cancelBtn->setStyleSheet("QPushButton { background: #262633; color: #e2e8f0; border-radius: 6px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background: #333344; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_primaryActionBtn = new QPushButton(tr("Start P2P Session & Inject Bridge"), this);
    m_primaryActionBtn->setStyleSheet("QPushButton { background: #6366f1; color: white; border-radius: 6px; padding: 8px 20px; font-weight: bold; } QPushButton:hover { background: #4f46e5; }");
    connect(m_primaryActionBtn, &QPushButton::clicked, this, &SVLMultiplayerHostDialog::onActionPrimaryClicked);

    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_primaryActionBtn);
    mainLayout->addLayout(btnLayout);
}

void SVLMultiplayerHostDialog::onModeToggled() {
    bool isP2P = m_radioP2P->isChecked();
    m_maxPlayersSpin->setVisible(isP2P);
    m_serverSubdomainEdit->setVisible(!isP2P);

    if (isP2P) {
        m_primaryActionBtn->setText(tr("Start P2P Session & Inject Bridge"));
        m_primaryActionBtn->setStyleSheet("QPushButton { background: #6366f1; color: white; border-radius: 6px; padding: 8px 20px; font-weight: bold; } QPushButton:hover { background: #4f46e5; }");
    } else {
        m_primaryActionBtn->setText(tr("Spin Up Cloud Server"));
        m_primaryActionBtn->setStyleSheet("QPushButton { background: #9333ea; color: white; border-radius: 6px; padding: 8px 20px; font-weight: bold; } QPushButton:hover { background: #7e22ce; }");
    }
}

void SVLMultiplayerHostDialog::onActionPrimaryClicked() {
    if (!m_hostingManager || !m_instance) return;

    m_progressBar->setVisible(true);
    m_statusLabel->setVisible(true);
    m_primaryActionBtn->setEnabled(false);

    if (m_radioP2P->isChecked()) {
        m_statusLabel->setText(tr("Injecting bridge mod and reserving tunnel route..."));
        m_statusLabel->setStyleSheet("color: #38bdf8;");

        QString shareCode;
        m_hostingManager->prepareP2PHosting(m_instance, m_worldNameEdit->text().trimmed(), m_maxPlayersSpin->value(), &shareCode);
    } else {
        m_statusLabel->setText(tr("Contacting Sunveil Cloud API to spin up server..."));
        m_statusLabel->setStyleSheet("color: #a855f7;");

        m_hostingManager->requestCloudServerCreation(
            m_worldNameEdit->text().trimmed(),
            m_serverSubdomainEdit->text().trimmed(),
            "" // uses auth headers from security
        );
    }
}

void SVLMultiplayerHostDialog::onP2PShareCodeReady(const QString& shareCode, const QString& inviteLink) {
    m_progressBar->setVisible(false);
    m_primaryActionBtn->setEnabled(true);
    m_lastInviteLink = inviteLink;

    m_statusLabel->setText(tr("✓ Sunveil Bridge injected! Ready to launch singleplayer world."));
    m_statusLabel->setStyleSheet("color: #10b981; font-weight: bold;");

    m_shareCodeLabel->setText(tr("Share Code: %1").arg(shareCode));
    m_shareCodeLabel->setVisible(true);
    m_copyInviteBtn->setVisible(true);

    m_primaryActionBtn->setText(tr("Launch World Now"));
    disconnect(m_primaryActionBtn, &QPushButton::clicked, this, &SVLMultiplayerHostDialog::onActionPrimaryClicked);
    connect(m_primaryActionBtn, &QPushButton::clicked, this, [this]() {
        emit readyToLaunchHostedInstance(m_instance);
        accept();
    });
}

void SVLMultiplayerHostDialog::onCloudServerCreated() {
    m_progressBar->setVisible(false);
    m_primaryActionBtn->setEnabled(true);

    m_statusLabel->setText(tr("✓ Cloud Server successfully provisioned on Sunveil Realms!"));
    m_statusLabel->setStyleSheet("color: #10b981; font-weight: bold;");

    QMessageBox::information(this, tr("Cloud Server Created"),
        tr("Your dedicated cloud server has been created and will appear on your Sunveil Realms tab."));
    accept();
}

void SVLMultiplayerHostDialog::onHostingError(const QString& error) {
    m_progressBar->setVisible(false);
    m_primaryActionBtn->setEnabled(true);

    m_statusLabel->setText(tr("Error: %1").arg(error));
    m_statusLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
}

void SVLMultiplayerHostDialog::copyInviteToClipboard() {
    if (!m_lastInviteLink.isEmpty()) {
        QGuiApplication::clipboard()->setText(m_lastInviteLink);
        QMessageBox::information(this, tr("Copied"), tr("Invite link copied to clipboard:\n%1").arg(m_lastInviteLink));
    }
}
