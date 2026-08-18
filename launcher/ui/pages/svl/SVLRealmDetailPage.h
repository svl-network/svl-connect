#pragma once

#include <QWidget>
#include "SVLConnectPage.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QFrame;
class QTextBrowser;

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
    void loadBanner(const QString& bannerUrl);
    void applyBannerPixmap(const QPixmap& originalPixmap);

private:
    SVLServerModel m_server;

    // Header widgets
    QPushButton* m_backBtn = nullptr;
    QLabel* m_headerTitle = nullptr;

    // Banner
    QFrame* m_bannerFrame = nullptr;
    QLabel* m_bannerBgLabel = nullptr;
    QLabel* m_bannerBadge = nullptr;

    // Left Column
    QLabel* m_iconLabel = nullptr;
    QLabel* m_serverNameLabel = nullptr;
    QLabel* m_playerCountBadge = nullptr;
    QLabel* m_latencyBadge = nullptr;
    QLabel* m_regionBadge = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QWidget* m_linksContainer = nullptr;
    QVBoxLayout* m_linksLayout = nullptr;

    // Right Column
    QLabel* m_ipValLabel = nullptr;
    QLabel* m_versionValLabel = nullptr;
    QLabel* m_loaderValLabel = nullptr;
    QLabel* m_statusValLabel = nullptr;
    QTextBrowser* m_descBrowser = nullptr;
    QWidget* m_modsContainer = nullptr;
    QVBoxLayout* m_modsLayout = nullptr;
    QLabel* m_modsHeaderLabel = nullptr;
};
