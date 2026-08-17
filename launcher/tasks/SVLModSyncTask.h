#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QUrl>
#include <memory>

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "ui/dialogs/SVLQuarantineDialog.h"

class MinecraftInstance;
class QNetworkAccessManager;
class QNetworkReply;

class SVLModSyncTask : public Task {
    Q_OBJECT

public:
    explicit SVLModSyncTask(const QString& masterApiBaseUrl,
                            const QString& serverKey,
                            const QString& serverName,
                            const QString& serverIp,
                            quint16 serverPort,
                            QWidget* parentWidget = nullptr);
    ~SVLModSyncTask() override;

    void executeTask() override;
    bool canAbort() const override { return true; }
    bool abort() override;

signals:
    void readyToLaunch(MinecraftInstance* instance, const QString& ip, quint16 port);

private slots:
    void onManifestReceived();
    void onDownloadsSucceeded();
    void onDownloadsFailed(const QString& reason);

private:
    void processManifest(const QByteArray& data);
    bool prepareInstance(const QString& mcVersion, const QString& loader, const QString& loaderVersion);
    void performCleanSyncAndDownload();
    void finalizeAndLaunch();

    QString m_masterApiBaseUrl;
    QString m_serverKey;
    QString m_serverName;
    QString m_serverIp;
    quint16 m_serverPort;
    QWidget* m_parentWidget = nullptr;

    bool m_aborted = false;

    // Manifest details
    QString m_mcVersion = "1.21.1";
    QString m_loader = "fabric";
    QString m_loaderVersion = "0.16.9";
    bool m_verified = true;
    QList<SVLModEntry> m_manifestMods;

    // Sync state
    QString m_modsDirPath;
    QList<SVLModEntry> m_modsToDownload;
    QNetworkReply* m_manifestReply = nullptr;
    NetJob::Ptr m_netJob;

    MinecraftInstance* m_instance = nullptr;
};
