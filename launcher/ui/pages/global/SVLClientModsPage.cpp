#include "SVLClientModsPage.h"
#include "settings/SettingsObject.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"

#include <QScrollArea>
#include <QStyle>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

SVLClientModsPage::SVLClientModsPage(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SVLClientModsPage::setupUI()
{
    // Two-way sync: Pull latest settings from active instance config if present
    if (APPLICATION->instances()) {
        for (int i = 0; i < APPLICATION->instances()->count(); ++i) {
            auto* inst = APPLICATION->instances()->at(i);
            if (inst) {
                syncInstanceToSettings(inst->gameRoot());
            }
        }
    }

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

    auto* headerSub = new QLabel(tr("Configure built-in enhancements before launch. Settings automatically synchronize two-way with the in-game client mod and update in real-time."), container);
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

    // 7. Emotes Suite (Emotecraft Integration)
    mainLayout->addWidget(createModRow(
        tr("Emote Studio (Emotecraft Suite)"),
        tr("Dynamic full-body player animations (Wave, Dance, Clap, Bow, Sit, Point) synced with all Sunveil clients (Key B)."),
        "ClientMod_Emotes",
        &m_emotesCheck,
        "ANIMATION"
    ));

    // 8. Ingame Merch & Cosmetics Shop
    mainLayout->addWidget(createModRow(
        tr("Ingame Merch & Cosmetics Shop"),
        tr("Custom player capes, animated 3D hats, wings, and custom weapon models catalog with in-game purchasing and equipping."),
        "ClientMod_Cosmetics",
        &m_cosmeticsCheck,
        "COSMETICS"
    ));

    // 9. 3D Audio Radar
    mainLayout->addWidget(createModRow(
        tr("3D Directional Audio Radar"),
        tr("Real-time directional sound radar HUD overlay with frequency filtering for footstep and combat detection."),
        "ClientMod_AudioRadar",
        &m_audioRadarCheck,
        "PVP"
    ));

    // 10. Keystrokes & CPS Overlay
    mainLayout->addWidget(createModRow(
        tr("Keystrokes & CPS Counter"),
        tr("Displays real-time WASD keys, Spacebar, LMB, RMB, and clicks per second on screen."),
        "ClientMod_Keystrokes",
        &m_keystrokesCheck,
        "HUD"
    ));

    // 11. Fullbright
    mainLayout->addWidget(createModRow(
        tr("Fullbright (Gamma Boost)"),
        tr("Overrides cave and nighttime darkness with 100% crystal clear vision in all dimensions."),
        "ClientMod_Fullbright",
        &m_fullbrightCheck,
        "VISUAL"
    ));

    mainLayout->addWidget(createModRow(
        tr("Freelook: Decoupled Camera"),
        tr("Free 360° camera orbit without rotating character body (walk straight while looking behind)."),
        "ClientMod_FreelookDecouple",
        &m_freelookDecoupleCheck,
        "CAMERA"
    ));

    mainLayout->addWidget(createModRow(
        tr("Look Nickname (Self Nametag)"),
        tr("Renders your own nametag/nickname above your head in 3rd person (F5) and during freelook."),
        "ClientMod_LookNickname",
        &m_lookNicknameCheck,
        "VISUAL"
    ));

    mainLayout->addWidget(createModRow(
        tr("Controlify (Controller Support)"),
        tr("Full gamepad & controller support for Xbox, PlayStation, Switch Pro and generic gamepads."),
        "ClientMod_Controlify",
        &m_controlifyCheck,
        "INPUT"
    ));

    mainLayout->addWidget(createModRow(
        tr("Wavey Capes"),
        tr("Physics-based cloth simulation with undulating waves and dynamic wind animations for capes."),
        "ClientMod_WaveyCapes",
        &m_waveyCapesCheck,
        "COSMETIC"
    ));

    mainLayout->addWidget(createModRow(
        tr("3D Skin Layers"),
        tr("Extrudes the second layer of player skins (hats, sleeves, jackets, pants) with 3D voxel depth."),
        "ClientMod_3DSkinLayers",
        &m_skinLayers3DCheck,
        "VISUAL"
    ));

    mainLayout->addWidget(createModRow(
        tr("Chat Heads"),
        tr("Renders player skin head avatar icons next to chat messages in the chat HUD."),
        "ClientMod_ChatHeads",
        &m_chatHeadsCheck,
        "HUD"
    ));

    mainLayout->addWidget(createModRow(
        tr("TAB Heads"),
        tr("Guarantees player avatar faces in the TAB player list across online and offline servers."),
        "ClientMod_TABHeads",
        &m_tabHeadsCheck,
        "HUD"
    ));

    mainLayout->addWidget(createModRow(
        tr("Clumps (XP Lag Fix)"),
        tr("Clumps nearby experience orbs together to drastically reduce entity lag and boost FPS."),
        "ClientMod_Clumps",
        &m_clumpsCheck,
        "PERF"
    ));

    mainLayout->addWidget(createModRow(
        tr("Shulker Box Tooltip"),
        tr("Interactive 3x9 grid tooltip preview showing all items inside shulker boxes on hover."),
        "ClientMod_ShulkerTooltip",
        &m_shulkerTooltipCheck,
        "UTILITY"
    ));

    mainLayout->addWidget(createModRow(
        tr("BetterF3"),
        tr("Clean, categorized, and color-coded modern F3 debug HUD cards replacing cluttered vanilla text."),
        "ClientMod_BetterF3",
        &m_betterF3Check,
        "HUD"
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
    if (m_emotesCheck) APPLICATION->settings()->set("ClientMod_Emotes", m_emotesCheck->isChecked());
    if (m_cosmeticsCheck) APPLICATION->settings()->set("ClientMod_Cosmetics", m_cosmeticsCheck->isChecked());
    if (m_audioRadarCheck) APPLICATION->settings()->set("ClientMod_AudioRadar", m_audioRadarCheck->isChecked());
    if (m_keystrokesCheck) APPLICATION->settings()->set("ClientMod_Keystrokes", m_keystrokesCheck->isChecked());
    if (m_fullbrightCheck) APPLICATION->settings()->set("ClientMod_Fullbright", m_fullbrightCheck->isChecked());
    if (m_freelookDecoupleCheck) APPLICATION->settings()->set("ClientMod_FreelookDecouple", m_freelookDecoupleCheck->isChecked());
    if (m_lookNicknameCheck) APPLICATION->settings()->set("ClientMod_LookNickname", m_lookNicknameCheck->isChecked());
    if (m_controlifyCheck) APPLICATION->settings()->set("ClientMod_Controlify", m_controlifyCheck->isChecked());
    if (m_waveyCapesCheck) APPLICATION->settings()->set("ClientMod_WaveyCapes", m_waveyCapesCheck->isChecked());
    if (m_skinLayers3DCheck) APPLICATION->settings()->set("ClientMod_3DSkinLayers", m_skinLayers3DCheck->isChecked());
    if (m_chatHeadsCheck) APPLICATION->settings()->set("ClientMod_ChatHeads", m_chatHeadsCheck->isChecked());
    if (m_tabHeadsCheck) APPLICATION->settings()->set("ClientMod_TABHeads", m_tabHeadsCheck->isChecked());
    if (m_clumpsCheck) APPLICATION->settings()->set("ClientMod_Clumps", m_clumpsCheck->isChecked());
    if (m_shulkerTooltipCheck) APPLICATION->settings()->set("ClientMod_ShulkerTooltip", m_shulkerTooltipCheck->isChecked());
    if (m_betterF3Check) APPLICATION->settings()->set("ClientMod_BetterF3", m_betterF3Check->isChecked());

    // Two-way synchronization: Propagate launcher settings into all instance configs
    if (APPLICATION->instances()) {
        for (int i = 0; i < APPLICATION->instances()->count(); ++i) {
            auto* inst = APPLICATION->instances()->at(i);
            if (inst) {
                syncSettingsToInstance(inst->gameRoot());
            }
        }
    }
    return true;
}

void SVLClientModsPage::syncSettingsToInstance(const QString& gameRoot)
{
    if (gameRoot.isEmpty()) return;
    QString configPath = QDir(gameRoot).filePath("config/sunveil-client.json");
    QFileInfo fi(configPath);
    fi.dir().mkpath(".");

    QJsonObject rootObj;
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        rootObj = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
    }

    QJsonObject modules = rootObj["modules"].toObject();
    auto settings = APPLICATION->settings();
    modules["perspective"] = settings->get("ClientMod_AltLook").toBool();
    modules["freecam"] = settings->get("ClientMod_Freecam").toBool();
    modules["waypoints"] = settings->get("ClientMod_Minimap").toBool();
    modules["itemPhysic"] = settings->get("ClientMod_ItemPhysics").toBool();
    modules["zoom"] = settings->get("ClientMod_FovZoom").toBool();
    modules["fps"] = settings->get("ClientMod_Performance").toBool();
    modules["emotes"] = settings->get("ClientMod_Emotes").toBool();
    modules["cosmetics"] = settings->get("ClientMod_Cosmetics").toBool();
    modules["audioRadar"] = settings->get("ClientMod_AudioRadar").toBool();
    modules["keystrokes"] = settings->get("ClientMod_Keystrokes").toBool();
    modules["brightness"] = settings->get("ClientMod_Fullbright").toBool();
    modules["freelookDecouple"] = settings->get("ClientMod_FreelookDecouple").toBool();
    modules["lookNickname"] = settings->get("ClientMod_LookNickname").toBool();
    modules["controlify"] = settings->get("ClientMod_Controlify").toBool();
    modules["waveyCapes"] = settings->get("ClientMod_WaveyCapes").toBool();
    modules["skinLayers3D"] = settings->get("ClientMod_3DSkinLayers").toBool();
    modules["chatHeads"] = settings->get("ClientMod_ChatHeads").toBool();
    modules["tabHeads"] = settings->get("ClientMod_TABHeads").toBool();
    modules["clumps"] = settings->get("ClientMod_Clumps").toBool();
    modules["shulkerBoxTooltip"] = settings->get("ClientMod_ShulkerTooltip").toBool();
    modules["betterF3"] = settings->get("ClientMod_BetterF3").toBool();

    rootObj["modules"] = modules;
    rootObj["enableCapes"] = settings->get("ClientMod_Cosmetics").toBool();
    rootObj["freelookDecouple"] = settings->get("ClientMod_FreelookDecouple").toBool();
    rootObj["lookNickname"] = settings->get("ClientMod_LookNickname").toBool();
    rootObj["controlify"] = settings->get("ClientMod_Controlify").toBool();
    rootObj["waveyCapes"] = settings->get("ClientMod_WaveyCapes").toBool();
    rootObj["skinLayers3D"] = settings->get("ClientMod_3DSkinLayers").toBool();
    rootObj["chatHeads"] = settings->get("ClientMod_ChatHeads").toBool();
    rootObj["tabHeads"] = settings->get("ClientMod_TABHeads").toBool();
    rootObj["clumps"] = settings->get("ClientMod_Clumps").toBool();
    rootObj["shulkerBoxTooltip"] = settings->get("ClientMod_ShulkerTooltip").toBool();
    rootObj["betterF3"] = settings->get("ClientMod_BetterF3").toBool();

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void SVLClientModsPage::syncInstanceToSettings(const QString& gameRoot)
{
    if (gameRoot.isEmpty()) return;
    QString configPath = QDir(gameRoot).filePath("config/sunveil-client.json");
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonObject rootObj = QJsonDocument::fromJson(file.readAll()).object();
    file.close();

    if (!rootObj.contains("modules")) return;
    QJsonObject modules = rootObj["modules"].toObject();
    auto settings = APPLICATION->settings();

    if (modules.contains("perspective")) settings->set("ClientMod_AltLook", modules["perspective"].toBool());
    if (modules.contains("freecam")) settings->set("ClientMod_Freecam", modules["freecam"].toBool());
    if (modules.contains("waypoints")) settings->set("ClientMod_Minimap", modules["waypoints"].toBool());
    if (modules.contains("itemPhysic")) settings->set("ClientMod_ItemPhysics", modules["itemPhysic"].toBool());
    if (modules.contains("zoom")) settings->set("ClientMod_FovZoom", modules["zoom"].toBool());
    if (modules.contains("fps")) settings->set("ClientMod_Performance", modules["fps"].toBool());
    if (modules.contains("emotes")) settings->set("ClientMod_Emotes", modules["emotes"].toBool());
    if (modules.contains("cosmetics")) settings->set("ClientMod_Cosmetics", modules["cosmetics"].toBool());
    if (modules.contains("audioRadar")) settings->set("ClientMod_AudioRadar", modules["audioRadar"].toBool());
    if (modules.contains("keystrokes")) settings->set("ClientMod_Keystrokes", modules["keystrokes"].toBool());
    if (modules.contains("brightness")) settings->set("ClientMod_Fullbright", modules["brightness"].toBool());
    if (modules.contains("freelookDecouple")) settings->set("ClientMod_FreelookDecouple", modules["freelookDecouple"].toBool());
    if (modules.contains("lookNickname")) settings->set("ClientMod_LookNickname", modules["lookNickname"].toBool());
    if (modules.contains("controlify")) settings->set("ClientMod_Controlify", modules["controlify"].toBool());
    if (modules.contains("waveyCapes")) settings->set("ClientMod_WaveyCapes", modules["waveyCapes"].toBool());
    if (modules.contains("skinLayers3D")) settings->set("ClientMod_3DSkinLayers", modules["skinLayers3D"].toBool());
    if (modules.contains("chatHeads")) settings->set("ClientMod_ChatHeads", modules["chatHeads"].toBool());
    if (modules.contains("tabHeads")) settings->set("ClientMod_TABHeads", modules["tabHeads"].toBool());
    if (modules.contains("clumps")) settings->set("ClientMod_Clumps", modules["clumps"].toBool());
    if (modules.contains("shulkerBoxTooltip")) settings->set("ClientMod_ShulkerTooltip", modules["shulkerBoxTooltip"].toBool());
    if (modules.contains("betterF3")) settings->set("ClientMod_BetterF3", modules["betterF3"].toBool());

    if (rootObj.contains("freelookDecouple")) settings->set("ClientMod_FreelookDecouple", rootObj["freelookDecouple"].toBool());
    if (rootObj.contains("lookNickname")) settings->set("ClientMod_LookNickname", rootObj["lookNickname"].toBool());
}
