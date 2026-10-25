#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MinecraftInstance;

struct WardrobeProfile {
    QString playerUuid;
    QString activeCapeId = "solar_emerald";
    QString customCapeUrl;
    bool enableCapes = true;
    bool waveyCapes = true;
    bool skinLayers3D = true;
    bool chatHeads = true;
    bool tabHeads = true;
    int sunveilCoins = 1000;
};

class SVLWardrobeManager : public QObject {
    Q_OBJECT

public:
    explicit SVLWardrobeManager(QNetworkAccessManager* nam, QObject* parent = nullptr);
    ~SVLWardrobeManager() override = default;

    // Load / Save active wardrobe settings from launcher configuration
    WardrobeProfile loadLocalProfile();
    void saveLocalProfile(const WardrobeProfile& profile);

    // Sync remote cape with Master API (/api/v1/capes/:uuid)
    void updateRemoteCape(const QString& uuid, const QString& capeIdentifier, const QByteArray& texturePngBytes = QByteArray());

    // Pre-launch hook: Writes preferences into config/sunveil-client.json in instance directory
    static bool applyCosmeticsToInstance(MinecraftInstance* instance, const WardrobeProfile& profile);

signals:
    void capeUpdated(bool success, const QString& message);

private slots:
    void onCapeReplyFinished();

private:
    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply* m_currentReply = nullptr;
    QString m_masterApiUrl = "https://realms.sunveil.net";
};
