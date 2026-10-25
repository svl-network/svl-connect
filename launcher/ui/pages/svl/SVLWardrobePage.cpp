#include "SVLWardrobePage.h"
#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFrame>

SVLWardrobePage::SVLWardrobePage(SVLWardrobeManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    setupUI();
    loadFromSettings();

    if (m_manager) {
        connect(m_manager, &SVLWardrobeManager::capeUpdated, this, [this](bool success, const QString& msg) {
            m_statusLabel->setText(success ? tr("✓ %1").arg(msg) : tr("⚠ %1").arg(msg));
            m_statusLabel->setStyleSheet(success ? "color: #10b981; font-weight: bold;" : "color: #ef4444; font-weight: bold;");
            m_statusLabel->setVisible(true);
        });
    }
}

void SVLWardrobePage::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(18);

    // Header
    auto* titleLayout = new QVBoxLayout();
    auto* title = new QLabel(tr("Wardrobe & Cosmetics"), this);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #ffffff;");
    auto* subtitle = new QLabel(tr("Equip official Sunveil Capes, 3D skin layers, and custom player cosmetics."), this);
    subtitle->setStyleSheet("font-size: 13px; color: #94a3b8;");
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);
    mainLayout->addLayout(titleLayout);

    // Cape Configuration Card
    auto* capeGroup = new QGroupBox(tr("Sunveil Cape Customization"), this);
    capeGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #38bdf8; border: 1px solid #282838; border-radius: 8px; margin-top: 10px; padding: 16px; background: #15151e; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 4px; }"
    );
    auto* capeLayout = new QVBoxLayout(capeGroup);
    capeLayout->setSpacing(12);

    m_enableCapesCheck = new QCheckBox(tr("Enable Sunveil Capes (Visible to all Sunveil players)"), capeGroup);
    m_enableCapesCheck->setChecked(true);
    m_enableCapesCheck->setStyleSheet("QCheckBox { color: #ffffff; font-size: 13px; }");
    capeLayout->addWidget(m_enableCapesCheck);

    auto* comboLabel = new QLabel(tr("Official Cape Design:"), capeGroup);
    comboLabel->setStyleSheet("color: #cbd5e1; font-size: 12px;");
    capeLayout->addWidget(comboLabel);

    m_capeCombo = new QComboBox(capeGroup);
    m_capeCombo->addItem(tr("Solar Emerald (Official)"), "solar_emerald");
    m_capeCombo->addItem(tr("Mystic Purple (Official)"), "mystic_purple");
    m_capeCombo->addItem(tr("Void Walker (Official)"), "void_walker");
    m_capeCombo->addItem(tr("Crimson Dragon (Official)"), "crimson_dragon");
    m_capeCombo->addItem(tr("Custom Cape URL..."), "custom");
    m_capeCombo->setStyleSheet("QComboBox { background: #1f1f2c; border: 1px solid #38384d; border-radius: 6px; padding: 8px; color: #fff; }");
    connect(m_capeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SVLWardrobePage::onCapeSelectionChanged);
    capeLayout->addWidget(m_capeCombo);

    m_customCapeUrlEdit = new QLineEdit(capeGroup);
    m_customCapeUrlEdit->setPlaceholderText(tr("https://example.com/my-cape.png"));
    m_customCapeUrlEdit->setStyleSheet("QLineEdit { background: #1f1f2c; border: 1px solid #38384d; border-radius: 6px; padding: 8px; color: #fff; }");
    m_customCapeUrlEdit->setVisible(false);
    capeLayout->addWidget(m_customCapeUrlEdit);

    mainLayout->addWidget(capeGroup);

    // Advanced Cosmetic Features Card
    auto* animGroup = new QGroupBox(tr("3D Skin & Animation Enhancements"), this);
    animGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #a855f7; border: 1px solid #282838; border-radius: 8px; margin-top: 10px; padding: 16px; background: #15151e; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 4px; }"
    );
    auto* animLayout = new QVBoxLayout(animGroup);
    animLayout->setSpacing(12);

    m_waveyCapesCheck = new QCheckBox(tr("Wavey Capes (Physics-based cape animation)"), animGroup);
    m_waveyCapesCheck->setChecked(true);
    m_waveyCapesCheck->setStyleSheet("QCheckBox { color: #ffffff; font-size: 13px; }");
    animLayout->addWidget(m_waveyCapesCheck);

    m_skinLayers3DCheck = new QCheckBox(tr("3D Skin Layers (Extruded skin details & armor layers)"), animGroup);
    m_skinLayers3DCheck->setChecked(true);
    m_skinLayers3DCheck->setStyleSheet("QCheckBox { color: #ffffff; font-size: 13px; }");
    animLayout->addWidget(m_skinLayers3DCheck);

    mainLayout->addWidget(animGroup);

    // Status banner
    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addStretch();

    // Save Button Bar
    auto* btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton(tr("Save & Sync to Active Profile"), this);
    m_saveBtn->setStyleSheet(
        "QPushButton { background: #6366f1; color: white; border-radius: 6px; padding: 10px 24px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background: #4f46e5; }"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &SVLWardrobePage::onSaveClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);
}

void SVLWardrobePage::onCapeSelectionChanged(int index) {
    QString selectedKey = m_capeCombo->itemData(index).toString();
    m_customCapeUrlEdit->setVisible(selectedKey == "custom");
}

WardrobeProfile SVLWardrobePage::currentProfile() const {
    WardrobeProfile p;
    p.enableCapes = m_enableCapesCheck->isChecked();
    p.activeCapeId = m_capeCombo->currentData().toString();
    p.customCapeUrl = m_customCapeUrlEdit->text().trimmed();
    p.waveyCapes = m_waveyCapesCheck->isChecked();
    p.skinLayers3D = m_skinLayers3DCheck->isChecked();
    return p;
}

void SVLWardrobePage::loadFromSettings() {
    if (!m_manager) return;
    WardrobeProfile p = m_manager->loadLocalProfile();
    m_enableCapesCheck->setChecked(p.enableCapes);
    m_waveyCapesCheck->setChecked(p.waveyCapes);
    m_skinLayers3DCheck->setChecked(p.skinLayers3D);
    m_customCapeUrlEdit->setText(p.customCapeUrl);

    int idx = m_capeCombo->findData(p.activeCapeId);
    if (idx >= 0) {
        m_capeCombo->setCurrentIndex(idx);
    } else {
        m_capeCombo->setCurrentIndex(0);
    }
    onCapeSelectionChanged(m_capeCombo->currentIndex());
}

void SVLWardrobePage::onSaveClicked() {
    if (!m_manager) return;
    WardrobeProfile p = currentProfile();
    m_manager->saveLocalProfile(p);

    // Get current account UUID if available
    QString uuid = "demo_user";
    if (APPLICATION->accounts() && APPLICATION->accounts()->defaultAccount()) {
        uuid = APPLICATION->accounts()->defaultAccount()->profileId();
    }

    m_manager->updateRemoteCape(uuid, p.activeCapeId);

    m_statusLabel->setText(tr("✓ Cosmetics saved locally and synced with Sunveil API."));
    m_statusLabel->setStyleSheet("color: #10b981; font-weight: bold;");
    m_statusLabel->setVisible(true);
}
