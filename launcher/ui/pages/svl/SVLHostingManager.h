#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

class MinecraftInstance;

struct CloudServerCreationResult {
    bool success = false;
    QString serverKey;
    QString serverName;
    QString ip = "realms.sunveil.net";
    quint16 port = 25565;
    QString licenseKey;
    QString errorMessage;
};

class SVLHostingManager : public QObject {
    Q_OBJECT

public:
    explicit SVLHostingManager(QNetworkAccessManager* nam, QObject* parent = nullptr);
    ~SVLHostingManager() override = default;

    // Option A: Configure Instance for P2P Local World Hosting (up to 8 players)
    bool prepareP2PHosting(MinecraftInstance* instance, const QString& worldName, int maxPlayers = 8, QString* shareCodeOut = nullptr);

    // Option B: Spin up 1-Click Cloud Server through Master API
    void requestCloudServerCreation(const QString& serverName, const QString& requestedSubdomain, const QString& authToken);

signals:
    void cloudServerCreated(const CloudServerCreationResult& result);
    void p2pShareCodeReady(const QString& shareCode, const QString& inviteLink);
    void hostingError(const QString& message);

private slots:
    void onCloudCreateReplyFinished();

private:
    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply* m_currentReply = nullptr;
    QString m_masterApiBaseUrl = "https://realms.sunveil.net";
};
