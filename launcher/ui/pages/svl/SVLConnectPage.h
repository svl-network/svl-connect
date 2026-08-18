#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <memory>

class QNetworkReply;
class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QScrollArea;
class MinecraftInstance;

#include <QHash>
#include <QPixmap>

struct SVLServerLinks {
    QString store;
    QString discord;
    QString website;
};

struct SVLServerModel {
    QString serverKey;
    QString name;
    QString icon;
    QString ip;
    quint16 port = 25565;
    QString mcVersion = "1.21.1";
    QString loader = "forge";
    QString loaderVersion;
    int players = 0;
    int maxPlayers = 20;
    QString motd;
    bool verified = true;
    int modCount = 13;
    bool isOnline = true;
    int boosts = 0;
    bool sponsored = false;
    QString bannerUrl;
    SVLServerLinks links;
};

class SVLConnectPage : public QWidget {
    Q_OBJECT

public:
    explicit SVLConnectPage(QWidget* parent = nullptr);
    ~SVLConnectPage() override;

    void refreshServers();
    void launchServer(const SVLServerModel& server);

    static QPixmap createRoundedIcon(const QPixmap& src, int width = 64, int height = 64, int radius = 8);
    static QPixmap loadServerIcon(const QString& iconData, int width = 64, int height = 64, int radius = 8);

signals:
    void launchRequested(MinecraftInstance* instance, const QString& ip, quint16 port);
    void serverDetailsRequested(const SVLServerModel& server);

private slots:
    void onServersReceived();
    void onSearchFilterChanged(const QString& query);
    void onConnectClicked(const SVLServerModel& server);

private:
    void setupUI();
    void renderServerCards();
    QWidget* createServerCard(const SVLServerModel& server);
    void bindServerIcon(const QString& iconData, QLabel* label, int size = 56);

    QString m_masterApiBaseUrl = "https://realms.sunveil.net";
    QList<SVLServerModel> m_allServers;
    QList<SVLServerModel> m_filteredServers;
    QString m_currentQuery;
    QHash<QString, QPixmap> m_iconCache;

    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QNetworkReply* m_currentReply = nullptr;
};


