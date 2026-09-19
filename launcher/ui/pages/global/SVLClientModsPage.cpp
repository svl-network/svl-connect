#include "SVLClientModsPage.h"

#include <QScrollArea>
#include <QStyle>

SVLClientModsPage::SVLClientModsPage(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SVLClientModsPage::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent; border: none;");

    auto* container = new QWidget(scroll);
    container->setStyleSheet("background: transparent; border: none;");
    auto* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    auto* headerTitle = new QLabel(tr("Sunveil Inbuilt Client Mods Suite"), container);
    headerTitle->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 800; border: none;");
    mainLayout->addWidget(headerTitle);

    auto* headerSub = new QLabel(tr("Enable or disable built-in quality of life client enhancements. Note: If a server explicitly forbids a mod (e.g. Freecam or Minimap), Sunveil Connect will automatically disable it for that session to adhere to server rules."), container);
    headerSub->setStyleSheet("color: #A1A1AA; font-size: 12px; line-height: 1.4; border: none;");
    headerSub->setWordWrap(true);
    mainLayout->addWidget(headerSub);

    mainLayout->addSpacing(8);

    // 1. Alt Look (Free Look / 360° Perspective)
    mainLayout->addWidget(createModRow(
        tr("Alt Look / 360° Perspective (Free Look)"),
        tr("Allows holding the Alt key to freely look around in third or first person without turning your player body."),
        "ClientMod_AltLook",
        &m_altLookCheck,
        "QOL"
    ));

    // 2. Freecam
    mainLayout->addWidget(createModRow(
        tr("Freecam (Detachable Camera)"),
        tr("Enables flying with a detached spectator camera while keeping player position static. Disabled automatically on competitive servers."),
        "ClientMod_Freecam",
        &m_freecamCheck,
        "CAMERA"
    ));

    // 3. Minimap & Waypoints
    mainLayout->addWidget(createModRow(
        tr("Minimap & Waypoints (Xaero's Minimap / WorldMap)"),
        tr("Provides a radar HUD minimap and waypoint markers with death points and customizable coordinate overlays."),
        "ClientMod_Minimap",
        &m_minimapCheck,
        "HUD"
    ));

    // 4. Item Physics
    mainLayout->addWidget(createModRow(
        tr("Item Physics (Realistic Dropped Items)"),
        tr("Replaces floating 2D sprite drops with full 3D physics-based tumbling and realistic block collisions."),
        "ClientMod_ItemPhysics",
        &m_itemPhysicsCheck,
        "VISUAL"
    ));

    // 5. FOV / Zoom Changer
    mainLayout->addWidget(createModRow(
        tr("Zoom & Dynamic FOV Changer"),
        tr("Provides smooth optical zoom keybinding (C) and dynamic field-of-view adjustments with customizable smoothing."),
        "ClientMod_FovZoom",
        &m_fovZoomCheck,
        "CONTROLS"
    ));

    // 6. Performance Suite
    mainLayout->addWidget(createModRow(
        tr("Performance & Shaders Suite (Sodium / Iris / Embeddium)"),
        tr("Next-generation modern rendering engine optimizations, multithreaded chunk loading, and shader pack support."),
        "ClientMod_Performance",
        &m_perfCheck,
        "PERFORMANCE"
    ));

    mainLayout->addStretch();
    scroll->setWidget(container);
    rootLayout->addWidget(scroll);
}

QWidget* SVLClientModsPage::createModRow(const QString& title, const QString& description, const QString& settingKey, QCheckBox** outBox, const QString& badgeText)
{
    auto* card = new QFrame(this);
    card->setObjectName("modCardFrame");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet("QFrame#modCardFrame { background-color: #1C1C1E; border: 1px solid #2C2C2E; border-radius: 10px; padding: 12px 16px; } QFrame#modCardFrame:hover { border: 1px solid #3F3F46; }");

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(14);

    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(3);

    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("color: #FFFFFF; font-size: 14px; font-weight: 700; border: none; background: transparent;");
    titleRow->addWidget(titleLabel);

    auto* badge = new QLabel(badgeText, card);
    badge->setStyleSheet("background-color: rgba(0, 229, 153, 0.12); color: #00E599; border: 1px solid rgba(0, 229, 153, 0.35); border-radius: 4px; padding: 2px 6px; font-size: 10px; font-weight: 800;");
    titleRow->addWidget(badge);
    titleRow->addStretch();

    textLayout->addLayout(titleRow);

    auto* descLabel = new QLabel(description, card);
    descLabel->setStyleSheet("color: #A1A1AA; font-size: 12px; border: none; background: transparent;");
    descLabel->setWordWrap(true);
    textLayout->addWidget(descLabel);

    layout->addLayout(textLayout, 1);

    auto* check = new QCheckBox(card);
    check->setCursor(Qt::PointingHandCursor);
    check->setStyleSheet("QCheckBox::indicator { width: 20px; height: 20px; border-radius: 4px; border: 1px solid #3F3F46; background-color: #27272A; } "
                         "QCheckBox::indicator:checked { background-color: #00E599; border-color: #00E599; image: none; }");
    bool isEnabled = APPLICATION->settings()->get(settingKey).toBool();
    check->setChecked(isEnabled);
    *outBox = check;

    layout->addWidget(check, 0, Qt::AlignVCenter);
    return card;
}

bool SVLClientModsPage::apply()
{
    if (m_altLookCheck) APPLICATION->settings()->set("ClientMod_AltLook", m_altLookCheck->isChecked());
    if (m_freecamCheck) APPLICATION->settings()->set("ClientMod_Freecam", m_freecamCheck->isChecked());
    if (m_minimapCheck) APPLICATION->settings()->set("ClientMod_Minimap", m_minimapCheck->isChecked());
    if (m_itemPhysicsCheck) APPLICATION->settings()->set("ClientMod_ItemPhysics", m_itemPhysicsCheck->isChecked());
    if (m_fovZoomCheck) APPLICATION->settings()->set("ClientMod_FovZoom", m_fovZoomCheck->isChecked());
    if (m_perfCheck) APPLICATION->settings()->set("ClientMod_Performance", m_perfCheck->isChecked());
    return true;
}
