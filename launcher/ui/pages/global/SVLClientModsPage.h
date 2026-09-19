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

private:
    void setupUI();
    QWidget* createModRow(const QString& title, const QString& description, const QString& settingKey, QCheckBox** outBox, const QString& badgeText = "CLIENT");

    QCheckBox* m_altLookCheck = nullptr;
    QCheckBox* m_freecamCheck = nullptr;
    QCheckBox* m_minimapCheck = nullptr;
    QCheckBox* m_itemPhysicsCheck = nullptr;
    QCheckBox* m_fovZoomCheck = nullptr;
    QCheckBox* m_perfCheck = nullptr;
};
