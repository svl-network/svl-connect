#include "SVLModSyncTask.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCryptographicHash>

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"

SVLModSyncTask::SVLModSyncTask(const QString& masterApiBaseUrl,
                               const QString& serverKey,
                               const QString& serverName,
                               const QString& serverIp,
                               quint16 serverPort,
                               QWidget* parentWidget)
    : Task(),
      m_masterApiBaseUrl(masterApiBaseUrl),
      m_serverKey(serverKey),
      m_serverName(serverName),
      m_serverIp(serverIp),
      m_serverPort(serverPort),
      m_parentWidget(parentWidget)
{
}

SVLModSyncTask::~SVLModSyncTask()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

bool SVLModSyncTask::abort()
{
    m_aborted = true;
    if (m_currentReply) {
        m_currentReply->abort();
    }
    emitAborted();
    return true;
}

void SVLModSyncTask::executeTask()
{
    setStatus(tr("Connecting to Sunveil Master API..."));
    setProgress(0, 100);

    if (m_masterApiBaseUrl.isEmpty()) {
        m_masterApiBaseUrl = "http://192.168.0.148:3001";
    }

    QUrl manifestUrl(m_masterApiBaseUrl + "/api/v1/servers/" + m_serverKey + "/manifest");
    QNetworkRequest request(manifestUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(10000);

    m_currentReply = APPLICATION->network()->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &SVLModSyncTask::onManifestReceived);
}

void SVLModSyncTask::onManifestReceived()
{
    if (m_aborted || !m_currentReply) {
        return;
    }

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emitFailed(tr("Failed to fetch server manifest from Master API: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    processManifest(data);
}

void SVLModSyncTask::processManifest(const QByteArray& data)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emitFailed(tr("Invalid server manifest JSON received from Master API."));
        return;
    }

    QJsonObject obj = doc.object();
    m_serverName = obj.value("name").toString(m_serverName);
    m_serverIp = obj.value("ip").toString(m_serverIp);
    m_serverPort = static_cast<quint16>(obj.value("port").toInt(m_serverPort > 0 ? m_serverPort : 25565));
    m_verified = obj.value("verified").toBool(false);

    QJsonObject verObj = obj.value("version").toObject();
    m_mcVersion = verObj.value("minecraft").toString("1.21.1");
    m_loader = verObj.value("loader").toString("fabric").toLower();
    m_loaderVersion = verObj.value("loaderVersion").toString("0.16.9");

    m_manifestMods.clear();
    QList<SVLModEntry> communityMods;

    QJsonArray modsArray = obj.value("mods").toArray();
    for (const QJsonValue& val : modsArray) {
        QJsonObject modObj = val.toObject();
        SVLModEntry entry;
        entry.projectId = modObj.value("projectId").toString();
        entry.fileName = modObj.value("fileName").toString();
        entry.sha256 = modObj.value("sha256").toString().toLower();
        entry.downloadUrl = modObj.value("downloadUrl").toString();
        entry.tier = modObj.value("tier").toString("official").toLower();

        if (entry.tier == "community" || (!entry.downloadUrl.startsWith("https://cdn.modrinth.com/"))) {
            entry.tier = "community";
            communityMods.append(entry);
        }

        m_manifestMods.append(entry);
    }

    // Security & Quarantine Check
    if (!m_verified || !communityMods.isEmpty()) {
        SVLQuarantineDialog dialog(m_serverName, m_serverKey, communityMods, m_parentWidget);
        if (dialog.exec() != QDialog::Accepted) {
            emitFailed(tr("Connection cancelled: Community mod quarantine was declined by the user."));
            return;
        }
    }

    setStatus(tr("Preparing local Minecraft instance..."));
    if (!prepareInstance(m_mcVersion, m_loader, m_loaderVersion)) {
        emitFailed(tr("Failed to create or configure local instance for server '%1'.").arg(m_serverKey));
        return;
    }

    performCleanSyncAndDownload();
}

bool SVLModSyncTask::prepareInstance(const QString& mcVersion, const QString& loader, const QString& loaderVersion)
{
    m_instance = APPLICATION->instances()->getInstanceById(m_serverKey);
    if (!m_instance) {
        QString baseDir = APPLICATION->instances()->primaryDir();
        if (baseDir.isEmpty()) {
            baseDir = FS::PathCombine(APPLICATION->dataRoot(), "instances");
        }
        QString instanceDir = FS::PathCombine(baseDir, m_serverKey);
        FS::ensureFolderPathExists(instanceDir);

        // 1. instance.cfg
        QString cfgPath = FS::PathCombine(instanceDir, "instance.cfg");
        QFile cfgFile(cfgPath);
        if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString content = QString("InstanceType=OneSix\n"
                                      "name=%1\n"
                                      "iconKey=default\n"
                                      "ManagedName=%2\n"
                                      "OverrideCommands=false\n")
                                  .arg(m_serverName, m_serverKey);
            cfgFile.write(content.toUtf8());
            cfgFile.close();
        }

        // 2. mmc-pack.json
        QString packPath = FS::PathCombine(instanceDir, "mmc-pack.json");
        QJsonObject packObj;
        packObj["formatVersion"] = 1;

        QJsonArray components;
        QJsonObject mcComponent;
        mcComponent["cachedName"] = "Minecraft";
        mcComponent["cachedRequires"] = QJsonArray();
        mcComponent["cachedVersion"] = mcVersion;
        mcComponent["important"] = true;
        mcComponent["uid"] = "net.minecraft";
        mcComponent["version"] = mcVersion;
        components.append(mcComponent);

        if (loader == "fabric") {
            QJsonObject fabricComp;
            fabricComp["cachedName"] = "Fabric Loader";
            fabricComp["cachedVersion"] = loaderVersion.isEmpty() ? "0.16.9" : loaderVersion;
            fabricComp["uid"] = "net.fabricmc.fabric-loader";
            fabricComp["version"] = loaderVersion.isEmpty() ? "0.16.9" : loaderVersion;
            components.append(fabricComp);
        } else if (loader == "neoforge") {
            QJsonObject neoComp;
            neoComp["cachedName"] = "NeoForge";
            neoComp["cachedVersion"] = loaderVersion.isEmpty() ? "21.1.70" : loaderVersion;
            neoComp["uid"] = "net.neoforged";
            neoComp["version"] = loaderVersion.isEmpty() ? "21.1.70" : loaderVersion;
            components.append(neoComp);
        } else if (loader == "forge") {
            QJsonObject forgeComp;
            forgeComp["cachedName"] = "Forge";
            forgeComp["cachedVersion"] = loaderVersion.isEmpty() ? "51.0.8" : loaderVersion;
            forgeComp["uid"] = "net.minecraftforge";
            forgeComp["version"] = loaderVersion.isEmpty() ? "51.0.8" : loaderVersion;
            components.append(forgeComp);
        }

        packObj["components"] = components;
        QFile packFile(packPath);
        if (packFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            packFile.write(QJsonDocument(packObj).toJson(QJsonDocument::Indented));
            packFile.close();
        }

        // Create minecraft/mods directory
        QString modsFolder = FS::PathCombine(instanceDir, ".minecraft", "mods");
        FS::ensureFolderPathExists(modsFolder);

        APPLICATION->instances()->loadList();
        m_instance = APPLICATION->instances()->getInstanceById(m_serverKey);
    }

    if (!m_instance) {
        return false;
    }

    // Determine correct mods folder path
    QString root = m_instance->instanceRoot();
    QString primaryMods = FS::PathCombine(root, ".minecraft", "mods");
    QString altMods = FS::PathCombine(root, "minecraft", "mods");

    if (QDir(altMods).exists()) {
        m_modsDirPath = altMods;
    } else {
        m_modsDirPath = primaryMods;
    }

    FS::ensureFolderPathExists(m_modsDirPath);
    return true;
}

void SVLModSyncTask::performCleanSyncAndDownload()
{
    setStatus(tr("Verifying local mod hashes..."));

    QDir modsDir(m_modsDirPath);
    QStringList localFiles = modsDir.entryList(QStringList() << "*.jar", QDir::Files);

    QMap<QString, SVLModEntry> manifestBySha;
    QMap<QString, SVLModEntry> manifestByName;
    for (const auto& mod : m_manifestMods) {
        if (!mod.sha256.isEmpty()) {
            manifestBySha.insert(mod.sha256, mod);
        }
        manifestByName.insert(mod.fileName.toLower(), mod);
    }

    QSet<QString> localHashes;
    for (const QString& localFile : localFiles) {
        QString fullPath = modsDir.absoluteFilePath(localFile);
        QFile file(fullPath);
        if (file.open(QIODevice::ReadOnly)) {
            QString hash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex().toLower();
            file.close();

            if (!manifestBySha.contains(hash)) {
                // Delete rogue or outdated local jar (Clean Sync)
                qDebug() << "[SVLModSync] Deleting local unlisted/outdated jar:" << localFile;
                QFile::remove(fullPath);
            } else {
                localHashes.insert(hash);
            }
        }
    }

    // Determine missing mods
    m_modsToDownload.clear();
    for (const auto& mod : m_manifestMods) {
        if (mod.downloadUrl.isEmpty()) {
            continue;
        }
        if (!localHashes.contains(mod.sha256)) {
            m_modsToDownload.append(mod);
        }
    }

    if (m_modsToDownload.isEmpty()) {
        qDebug() << "[SVLModSync] All mods are up-to-date and cryptographically verified.";
        finalizeAndLaunch();
        return;
    }

    m_downloadIndex = 0;
    setStatus(tr("Synchronizing %1 mod(s)...").arg(m_modsToDownload.size()));
    setProgress(0, m_modsToDownload.size());
    downloadNextMod();
}

void SVLModSyncTask::downloadNextMod()
{
    if (m_aborted) return;

    if (m_downloadIndex >= m_modsToDownload.size()) {
        finalizeAndLaunch();
        return;
    }

    const auto& mod = m_modsToDownload.at(m_downloadIndex);
    setStatus(tr("Downloading %1 (%2/%3)...")
                  .arg(mod.fileName, QString::number(m_downloadIndex + 1), QString::number(m_modsToDownload.size())));
    setProgress(m_downloadIndex, m_modsToDownload.size());

    QUrl url(mod.downloadUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(30000);

    m_currentReply = APPLICATION->network()->get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &SVLModSyncTask::onModDownloadFinished);
}

void SVLModSyncTask::onModDownloadFinished()
{
    if (m_aborted || !m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emitFailed(tr("Failed to download mod '%1': %2")
                       .arg(m_modsToDownload.at(m_downloadIndex).fileName, reply->errorString()));
        return;
    }

    const auto& mod = m_modsToDownload.at(m_downloadIndex);
    QByteArray fileData = reply->readAll();

    // Verify SHA-256 before saving
    QString computedSha256 = QCryptographicHash::hash(fileData, QCryptographicHash::Sha256).toHex().toLower();
    if (!mod.sha256.isEmpty() && computedSha256 != mod.sha256) {
        emitFailed(tr("SECURITY ALERT: SHA-256 mismatch for '%1'!\nExpected: %2\nComputed: %3")
                       .arg(mod.fileName, mod.sha256, computedSha256));
        return;
    }

    QString targetPath = FS::PathCombine(m_modsDirPath, mod.fileName);
    QString partPath = targetPath + ".part";

    QFile partFile(partPath);
    if (!partFile.open(QIODevice::WriteOnly)) {
        emitFailed(tr("Cannot write to file '%1'.").arg(partPath));
        return;
    }
    partFile.write(fileData);
    partFile.close();

    if (QFile::exists(targetPath)) {
        QFile::remove(targetPath);
    }
    if (!QFile::rename(partPath, targetPath)) {
        emitFailed(tr("Failed to save mod '%1'.").arg(targetPath));
        return;
    }

    qDebug() << "[SVLModSync] Verified and installed mod:" << mod.fileName;
    m_downloadIndex++;
    downloadNextMod();
}

void SVLModSyncTask::finalizeAndLaunch()
{
    setStatus(tr("Synchronization complete. Ready to launch."));
    setProgress(100, 100);

    emit readyToLaunch(m_instance, m_serverIp, m_serverPort);
    emitSucceeded();
}
