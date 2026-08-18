#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class QNetworkReply;

struct SVLClientUpdateInfo {
    QString version;
    bool mandatory = false;
    QString downloadUrl;
    QString changelog;
};

struct SVLBridgeUpdateInfo {
    QString version;
    QString downloadUrl;
};

class SVLUpdateManager : public QObject {
    Q_OBJECT

public:
    static SVLUpdateManager* instance();

    void checkForUpdates(int delayMs = 3000);
    bool hasUpdateAvailable() const { return m_updateAvailable; }
    SVLClientUpdateInfo clientUpdateInfo() const { return m_clientInfo; }
    SVLBridgeUpdateInfo bridgeUpdateInfo() const { return m_bridgeInfo; }

    static bool isVersionNewer(const QString& remoteVersion, const QString& currentVersion);

signals:
    void updateAvailable(const QString& newVersion, bool isMandatory, const QString& downloadUrl, const QString& changelog);

private slots:
    void onCheckUpdatesTriggered();
    void onUpdateReplyFinished();

private:
    explicit SVLUpdateManager(QObject* parent = nullptr);
    ~SVLUpdateManager() override = default;

private:
    static SVLUpdateManager* s_instance;
    bool m_updateAvailable = false;
    SVLClientUpdateInfo m_clientInfo;
    SVLBridgeUpdateInfo m_bridgeInfo;
    QNetworkReply* m_reply = nullptr;
};
