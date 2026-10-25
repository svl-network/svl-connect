#pragma once

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "SVLWardrobeManager.h"

class SVLWardrobePage : public QWidget {
    Q_OBJECT

public:
    explicit SVLWardrobePage(SVLWardrobeManager* manager, QWidget* parent = nullptr);
    ~SVLWardrobePage() override = default;

    WardrobeProfile currentProfile() const;

private slots:
    void onSaveClicked();
    void onCapeSelectionChanged(int index);

private:
    void setupUI();
    void loadFromSettings();

    SVLWardrobeManager* m_manager = nullptr;
    QComboBox* m_capeCombo = nullptr;
    QLineEdit* m_customCapeUrlEdit = nullptr;
    QCheckBox* m_enableCapesCheck = nullptr;
    QCheckBox* m_waveyCapesCheck = nullptr;
    QCheckBox* m_skinLayers3DCheck = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_capePreview = nullptr;
};
