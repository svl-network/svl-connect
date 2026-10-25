#pragma once

#include <QDialog>
#include <QString>
#include <memory>

class MinecraftInstance;
class SVLHostingManager;
class QRadioButton;
class QLineEdit;
class QPushButton;
class QLabel;
class QSpinBox;
class QProgressBar;

class SVLMultiplayerHostDialog : public QDialog {
    Q_OBJECT

public:
    explicit SVLMultiplayerHostDialog(MinecraftInstance* instance, SVLHostingManager* hostingManager, QWidget* parent = nullptr);
    ~SVLMultiplayerHostDialog() override = default;

    static void showHostDialog(MinecraftInstance* instance, SVLHostingManager* hostingManager, QWidget* parent = nullptr);

signals:
    void readyToLaunchHostedInstance(MinecraftInstance* instance);

private slots:
    void onModeToggled();
    void onActionPrimaryClicked();
    void onP2PShareCodeReady(const QString& shareCode, const QString& inviteLink);
    void onCloudServerCreated();
    void onHostingError(const QString& error);
    void copyInviteToClipboard();

private:
    void setupUI();

    MinecraftInstance* m_instance = nullptr;
    SVLHostingManager* m_hostingManager = nullptr;

    QRadioButton* m_radioP2P = nullptr;
    QRadioButton* m_radioCloud = nullptr;

    QLineEdit* m_worldNameEdit = nullptr;
    QSpinBox* m_maxPlayersSpin = nullptr;
    QLineEdit* m_serverSubdomainEdit = nullptr;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_shareCodeLabel = nullptr;
    QPushButton* m_copyInviteBtn = nullptr;
    QPushButton* m_primaryActionBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QProgressBar* m_progressBar = nullptr;

    QString m_lastInviteLink;
};
