#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

#include "ui/pages/BasePage.h"
#include "Application.h"

class SVLClientModsPage : public QWidget, public BasePage {
    Q_OBJECT

public:
    explicit SVLClientModsPage(QWidget* parent = nullptr);
    ~SVLClientModsPage() override = default;

    QString id() const override { return "svl_client_mods"; }
    QString displayName() const override { return tr("Client Mods"); }
    QIcon icon() const override { return QIcon::fromTheme("centralmods"); }

    bool apply() override;
    void retranslate() override {}

    static void syncSettingsToInstance(const QString& gameRoot);
    static void syncInstanceToSettings(const QString& gameRoot);
    static void performPostGameAutoUpdate(const QString& gameRoot);
    static void checkForLauncherUpdates();
    static void applyPendingLauncherUpdateOnExit();

private:
    void setupUI();
    QWidget* createModRow(const QString& title, const QString& description, const QString& settingKey, QCheckBox** outBox, const QString& badgeText = "CLIENT");

    QCheckBox* m_autoUpdateCheck = nullptr;
    QCheckBox* m_altLookCheck = nullptr;
    QCheckBox* m_freecamCheck = nullptr;
    QCheckBox* m_minimapCheck = nullptr;
    QCheckBox* m_itemPhysicsCheck = nullptr;
    QCheckBox* m_fovZoomCheck = nullptr;
    QCheckBox* m_perfCheck = nullptr;
    QCheckBox* m_emotesCheck = nullptr;
    QCheckBox* m_cosmeticsCheck = nullptr;
    QCheckBox* m_audioRadarCheck = nullptr;
    QCheckBox* m_keystrokesCheck = nullptr;
    QCheckBox* m_fullbrightCheck = nullptr;
    QCheckBox* m_freelookDecoupleCheck = nullptr;
    QCheckBox* m_lookNicknameCheck = nullptr;
    QCheckBox* m_controlifyCheck = nullptr;
    QCheckBox* m_waveyCapesCheck = nullptr;
    QCheckBox* m_skinLayers3DCheck = nullptr;
    QCheckBox* m_chatHeadsCheck = nullptr;
    QCheckBox* m_tabHeadsCheck = nullptr;
    QCheckBox* m_clumpsCheck = nullptr;
    QCheckBox* m_shulkerTooltipCheck = nullptr;
    QCheckBox* m_betterF3Check = nullptr;
};
