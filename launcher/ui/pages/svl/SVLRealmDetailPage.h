#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>

#include "SVLConnectPage.h"

class SVLRealmDetailPage : public QWidget {
    Q_OBJECT
public:
    explicit SVLRealmDetailPage(QWidget* parent = nullptr);
    ~SVLRealmDetailPage() override = default;

    void setServer(const SVLServerModel& server);
    const SVLServerModel& currentServer() const { return m_server; }

signals:
    void backRequested();
    void connectRequested(const SVLServerModel& server);

private:
    void setupUI();
    void updateUI();

private:
    SVLServerModel m_server;

    // Header widgets
    QPushButton* m_backBtn = nullptr;
    QLabel* m_headerTitle = nullptr;

    // Left Column
    QLabel* m_iconLabel = nullptr;
    QLabel* m_serverNameLabel = nullptr;
    QLabel* m_playerCountBadge = nullptr;
    QLabel* m_latencyBadge = nullptr;
    QLabel* m_regionBadge = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QPushButton* m_favBtn = nullptr;
    QPushButton* m_copyIpBtn = nullptr;
    QPushButton* m_discordBtn = nullptr;

    // Right Column
    QLabel* m_ipValLabel = nullptr;
    QLabel* m_versionValLabel = nullptr;
    QLabel* m_loaderValLabel = nullptr;
    QLabel* m_statusValLabel = nullptr;
    QLabel* m_motdTextLabel = nullptr;
    QWidget* m_modsContainer = nullptr;
    QVBoxLayout* m_modsLayout = nullptr;
    QLabel* m_modsHeaderLabel = nullptr;
};
