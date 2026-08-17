#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <memory>

class QNetworkReply;
class QLineEdit;
class QLabel;
class QVBoxLayout;
class QScrollArea;
class MinecraftInstance;

struct SVLServerModel {
    QString serverKey;
    QString name;
    QString ip;
    quint16 port = 25565;
    QString mcVersion = "1.21.1";
    QString loader = "fabric";
    QString loaderVersion;
    int players = 0;
    int maxPlayers = 20;
    QString motd;
    bool verified = false;
    int modCount = 0;
};

class SVLConnectPage : public QWidget {
    Q_OBJECT

public:
    explicit SVLConnectPage(QWidget* parent = nullptr);
    ~SVLConnectPage() override;

    void refreshServers();
    void launchServer(const SVLServerModel& server);

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

    QString m_masterApiBaseUrl = "http://192.168.0.148:3001";
    QList<SVLServerModel> m_allServers;
    QList<SVLServerModel> m_filteredServers;
    QString m_currentQuery;

    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QNetworkReply* m_currentReply = nullptr;
};
