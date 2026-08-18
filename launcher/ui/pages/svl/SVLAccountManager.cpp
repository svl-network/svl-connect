#include "SVLAccountManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFrame>
#include <QMessageBox>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>

#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "ui/dialogs/MSALoginDialog.h"

SVLAccountManager::SVLAccountManager(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    refreshAccountList();
}

void SVLAccountManager::showAccountManager(QWidget* parent)
{
    auto* manager = new SVLAccountManager(parent);
    manager->setAttribute(Qt::WA_DeleteOnClose);
    manager->exec();
}

void SVLAccountManager::setupUI()
{
    setObjectName("SVLAccountManager");
    setWindowTitle(tr("Account Manager - Sunveil Connect"));
    resize(640, 520);
    setMinimumSize(540, 440);
    setStyleSheet("QDialog#SVLAccountManager { background-color: #111111; color: #FFFFFF; font-family: 'Segoe UI', sans-serif; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    // 1. Header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    auto* titleLabel = new QLabel(tr("ACCOUNT MANAGER"), this);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 20px; font-weight: 800; letter-spacing: 0.5px; border: none;");
    titleLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(tr("Manage Microsoft and Offline (Cracked) Minecraft player profiles."), this);
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

    // 2. Active Account Indicator Card
    auto* activeCard = new QFrame(this);
    activeCard->setStyleSheet("background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px;");
    auto* activeLayout = new QHBoxLayout(activeCard);
    activeLayout->setContentsMargins(16, 12, 16, 12);
    activeLayout->setSpacing(12);

    auto* activeIcon = new QLabel(tr("👤"), activeCard);
    activeIcon->setStyleSheet("font-size: 24px; border: none; background: transparent;");
    activeLayout->addWidget(activeIcon);

    auto* activeInfoLayout = new QVBoxLayout();
    activeInfoLayout->setSpacing(2);
    auto* activeTitle = new QLabel(tr("ACTIVE PLAYER PROFILE"), activeCard);
    activeTitle->setStyleSheet("color: #00E599; font-size: 10px; font-weight: 800; letter-spacing: 0.5px; border: none;");
    activeInfoLayout->addWidget(activeTitle);

    m_activeAccountLabel = new QLabel(tr("No active account selected"), activeCard);
    m_activeAccountLabel->setStyleSheet("color: #FFFFFF; font-size: 15px; font-weight: 700; border: none;");
    activeInfoLayout->addWidget(m_activeAccountLabel);

    activeLayout->addLayout(activeInfoLayout, 1);
    mainLayout->addWidget(activeCard);

    // 3. Accounts List
    auto* listSectionTitle = new QLabel(tr("REGISTERED ACCOUNTS"), this);
    listSectionTitle->setStyleSheet("color: #A1A1AA; font-size: 11px; font-weight: 700; letter-spacing: 0.5px; border: none;");
    mainLayout->addWidget(listSectionTitle);

    m_accountListWidget = new QListWidget(this);
    m_accountListWidget->setStyleSheet("QListWidget { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px; padding: 6px; color: #FFFFFF; font-size: 13px; } QListWidget::item { background-color: #111111; border: 1px solid #2C2C2E; border-radius: 6px; padding: 10px; margin-bottom: 6px; } QListWidget::item:selected { background-color: #242426; border-color: #00E599; }");
    connect(m_accountListWidget, &QListWidget::itemSelectionChanged, this, &SVLAccountManager::onAccountSelectionChanged);
    mainLayout->addWidget(m_accountListWidget, 1);

    // List Action Buttons
    auto* listActionLayout = new QHBoxLayout();
    listActionLayout->setSpacing(8);

    m_setDefaultBtn = new QPushButton(tr("★ Set as Active Default"), this);
    m_setDefaultBtn->setEnabled(false);
    m_setDefaultBtn->setCursor(Qt::PointingHandCursor);
    m_setDefaultBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 6px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #3F3F46; } QPushButton:disabled { color: #52525B; }");
    connect(m_setDefaultBtn, &QPushButton::clicked, this, &SVLAccountManager::onSetDefaultClicked);
    listActionLayout->addWidget(m_setDefaultBtn);

    m_removeBtn = new QPushButton(tr("🗑 Remove Account"), this);
    m_removeBtn->setEnabled(false);
    m_removeBtn->setCursor(Qt::PointingHandCursor);
    m_removeBtn->setStyleSheet("QPushButton { background-color: #2C2C2E; color: #EF4444; border: 1px solid #2C2C2E; border-radius: 6px; padding: 8px 16px; font-weight: 700; font-size: 12px; } QPushButton:hover { background-color: #3F3F46; border-color: #EF4444; } QPushButton:disabled { color: #52525B; }");
    connect(m_removeBtn, &QPushButton::clicked, this, &SVLAccountManager::onRemoveAccountClicked);
    listActionLayout->addWidget(m_removeBtn);

    listActionLayout->addStretch();
    mainLayout->addLayout(listActionLayout);

    // 4. Add Accounts Section (Microsoft & Offline)
    auto* addCard = new QFrame(this);
    addCard->setStyleSheet("background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px;");
    auto* addLayout = new QVBoxLayout(addCard);
    addLayout->setContentsMargins(16, 14, 16, 14);
    addLayout->setSpacing(12);

    auto* addTitle = new QLabel(tr("ADD MINECRAFT ACCOUNT"), addCard);
    addTitle->setStyleSheet("color: #FFFFFF; font-size: 12px; font-weight: 800; letter-spacing: 0.5px; border: none;");
    addLayout->addWidget(addTitle);

    // Row 1: Microsoft Login
    auto* msaRow = new QHBoxLayout();
    msaRow->setSpacing(10);
    auto* msaLabel = new QLabel(tr("Official Microsoft Account (Online Multi-Server):"), addCard);
    msaLabel->setStyleSheet("color: #A1A1AA; font-size: 12px; border: none;");
    msaRow->addWidget(msaLabel, 1);

    m_msaLoginBtn = new QPushButton(tr("Sign in with Microsoft"), addCard);
    m_msaLoginBtn->setCursor(Qt::PointingHandCursor);
    m_msaLoginBtn->setStyleSheet("QPushButton { background-color: #0078D4; color: #FFFFFF; font-size: 12px; font-weight: 800; border: none; border-radius: 6px; padding: 8px 18px; } QPushButton:hover { background-color: #1084D9; }");
    connect(m_msaLoginBtn, &QPushButton::clicked, this, &SVLAccountManager::onMicrosoftLoginClicked);
    msaRow->addWidget(m_msaLoginBtn);
    addLayout->addLayout(msaRow);

    // Row 2: Offline / Cracked Login
    auto* offlineRow = new QHBoxLayout();
    offlineRow->setSpacing(10);

    m_offlineUsernameEdit = new QLineEdit(addCard);
    m_offlineUsernameEdit->setPlaceholderText(tr("Enter offline username (e.g. Steve, HeyTaxx)..."));
    m_offlineUsernameEdit->setStyleSheet("QLineEdit { background-color: #111111; color: #FFFFFF; border: 1px solid #2C2C2E; border-radius: 6px; padding: 8px 12px; font-size: 12px; } QLineEdit:focus { border-color: #00E599; }");
    connect(m_offlineUsernameEdit, &QLineEdit::returnPressed, this, &SVLAccountManager::onAddOfflineAccountClicked);
    offlineRow->addWidget(m_offlineUsernameEdit, 1);

    m_addOfflineBtn = new QPushButton(tr("Add Offline Account"), addCard);
    m_addOfflineBtn->setCursor(Qt::PointingHandCursor);
    m_addOfflineBtn->setStyleSheet("QPushButton { background-color: #00E599; color: #000000; font-size: 12px; font-weight: 900; border: none; border-radius: 6px; padding: 8px 18px; } QPushButton:hover { background-color: #10FFAC; }");
    connect(m_addOfflineBtn, &QPushButton::clicked, this, &SVLAccountManager::onAddOfflineAccountClicked);
    offlineRow->addWidget(m_addOfflineBtn);

    addLayout->addLayout(offlineRow);
    mainLayout->addWidget(addCard);

    // 5. Status message
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #71717A; font-size: 12px; font-weight: 600; border: none;");
    mainLayout->addWidget(m_statusLabel);
}

void SVLAccountManager::refreshAccountList()
{
    m_accountListWidget->clear();
    auto accounts = APPLICATION->accounts();
    if (!accounts) return;

    auto defaultAcc = accounts->defaultAccount();
    if (defaultAcc) {
        QString typeStr = (defaultAcc->accountType() == AccountType::MSA) ? tr("Microsoft Premium") : tr("Offline (UUID v3)");
        m_activeAccountLabel->setText(QString("%1 (%2)").arg(defaultAcc->profileName(), typeStr));
    } else {
        m_activeAccountLabel->setText(tr("No active account selected"));
    }

    for (int i = 0; i < accounts->count(); ++i) {
        auto acc = accounts->at(i);
        if (!acc) continue;

        bool isDefault = (acc == defaultAcc);
        QString typeTag = (acc->accountType() == AccountType::MSA) ? tr("[MICROSOFT]") : tr("[OFFLINE]");
        QString itemText = QString("%1 %2 %3").arg(acc->profileName(), typeTag, isDefault ? tr("★ ACTIVE DEFAULT") : "");

        auto* item = new QListWidgetItem(itemText, m_accountListWidget);
        item->setData(Qt::UserRole, i);
        if (isDefault) {
            item->setForeground(QColor("#00E599"));
        }
    }

    onAccountSelectionChanged();
}

void SVLAccountManager::onAccountSelectionChanged()
{
    bool hasSelection = (m_accountListWidget->currentRow() >= 0);
    m_setDefaultBtn->setEnabled(hasSelection);
    m_removeBtn->setEnabled(hasSelection);
}

void SVLAccountManager::onSetDefaultClicked()
{
    int row = m_accountListWidget->currentRow();
    auto accounts = APPLICATION->accounts();
    if (row >= 0 && accounts && row < accounts->count()) {
        auto acc = accounts->at(row);
        accounts->setDefaultAccount(acc);
        m_statusLabel->setText(tr("✓ Active default account set to '%1'").arg(acc->profileName()));
        refreshAccountList();
    }
}

void SVLAccountManager::onRemoveAccountClicked()
{
    int row = m_accountListWidget->currentRow();
    auto accounts = APPLICATION->accounts();
    if (row >= 0 && accounts && row < accounts->count()) {
        auto acc = accounts->at(row);
        QString name = acc->profileName();
        accounts->removeAccount(accounts->index(row, 0));
        m_statusLabel->setText(tr("✓ Removed account '%1'").arg(name));
        refreshAccountList();
    }
}

void SVLAccountManager::onMicrosoftLoginClicked()
{
    auto account = MSALoginDialog::newAccount(this);
    if (account) {
        auto accounts = APPLICATION->accounts();
        accounts->addAccount(account);
        if (accounts->count() == 1 || !accounts->defaultAccount()) {
            accounts->setDefaultAccount(account);
        }
        m_statusLabel->setText(tr("✓ Successfully linked Microsoft Account: %1").arg(account->profileName()));
        refreshAccountList();
    }
}

void SVLAccountManager::onAddOfflineAccountClicked()
{
    QString username = m_offlineUsernameEdit->text().trimmed();
    if (username.isEmpty()) {
        m_statusLabel->setText(tr("Please enter a valid username."));
        return;
    }

    if (username.length() < 2 || username.length() > 16) {
        m_statusLabel->setText(tr("Username must be between 2 and 16 characters."));
        return;
    }

    if (const MinecraftAccountPtr account = MinecraftAccount::createOffline(username)) {
        account->login()->start();
        auto accounts = APPLICATION->accounts();
        accounts->addAccount(account);
        if (accounts->count() == 1 || !accounts->defaultAccount()) {
            accounts->setDefaultAccount(account);
        }
        m_offlineUsernameEdit->clear();
        m_statusLabel->setText(tr("✓ Offline account '%1' created and ready for play.").arg(username));
        refreshAccountList();
    }
}
