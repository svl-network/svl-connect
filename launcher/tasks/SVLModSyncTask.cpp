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
#include "minecraft/PackProfile.h"
#include "net/ChecksumValidator.h"

#include <tag_compound.h>
#include <tag_list.h>
#include <tag_primitive.h>
#include <tag_string.h>
#include <io/stream_reader.h>
#include <io/stream_writer.h>
#include <sstream>

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
    if (m_manifestReply) {
        m_manifestReply->abort();
        m_manifestReply->deleteLater();
        m_manifestReply = nullptr;
    }
}

bool SVLModSyncTask::abort()
{
    m_aborted = true;
    if (m_manifestReply) {
        m_manifestReply->abort();
    }
    if (m_netJob) {
        m_netJob->abort();
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
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(10000);

    m_manifestReply = APPLICATION->network()->get(request);
    connect(m_manifestReply, &QNetworkReply::finished, this, &SVLModSyncTask::onManifestReceived);
}

void SVLModSyncTask::onManifestReceived()
{
    if (m_aborted || !m_manifestReply) {
        return;
    }

    QNetworkReply* reply = m_manifestReply;
    m_manifestReply = nullptr;
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
    m_mcVersion = verObj.value("minecraft").toString(verObj.value("minecraftVersion").toString("1.21.1"));
    m_loader = verObj.value("loader").toString(verObj.value("modLoader").toString("fabric")).toLower();
    m_loaderVersion = verObj.value("loaderVersion").toString(verObj.value("modLoaderVersion").toString(""));

    m_manifestMods.clear();
    QList<SVLModEntry> communityMods;

    // Check if the server platform is a pure plugin server
    bool isPluginServer = (m_loader == "paper" || m_loader == "spigot" || m_loader == "bukkit" ||
                           m_loader == "purpur" || m_loader == "folia" || m_loader == "velocity" ||
                           m_loader == "bungee" || m_loader == "bungeecord" || m_loader == "waterfall");

    static const QStringList serverOnlyKeywords = {
        "authme", "luckperms", "worldguard", "worldedit-bukkit", "protocollib",
        "viaversion", "viabackwards", "viarewind", "tradeshop", "discordsrv",
        "chunky", "floodgate", "geyser", "lagfixer", "skinsrestorer",
        "tebex", "veinminer-paper", "antigrief", "vault", "essentials",
        "clearlag", "multiverse", "citizens", "griefprevention", "coreprotect",
        "spark-bukkit", "placeholderapi", "dynmap", "bluemap", "squaremap",
        "decentholograms", "holographicdisplays", "chatex", "tab"
    };

    QJsonArray modsArray = obj.value("mods").toArray();
    for (const QJsonValue& val : modsArray) {
        QJsonObject modObj = val.toObject();
        SVLModEntry entry;
        entry.projectId = modObj.value("projectId").toString();
        entry.fileName = modObj.value("fileName").toString();
        entry.sha256 = modObj.value("sha256").toString().toLower();
        entry.downloadUrl = modObj.value("downloadUrl").toString();
        entry.tier = modObj.value("tier").toString("official").toLower();

        QString lowerFile = entry.fileName.toLower();
        bool isServerPlugin = isPluginServer ||
                              lowerFile.endsWith("-bukkit.jar") || lowerFile.endsWith("-spigot.jar") ||
                              lowerFile.endsWith("-paper.jar") || lowerFile.contains("-bukkit-") ||
                              lowerFile.contains("-spigot-") || lowerFile.contains("-paper-") ||
                              lowerFile.contains("bukkit") || lowerFile.contains("spigot");

        if (!isServerPlugin) {
            for (const auto& keyword : serverOnlyKeywords) {
                if (lowerFile.startsWith(keyword) || lowerFile.contains(keyword)) {
                    isServerPlugin = true;
                    break;
                }
            }
        }

        if (isServerPlugin) {
            qDebug() << "[SVLModSync] Excluding server-only plugin from client instance:" << entry.fileName;
            continue;
        }

        if (entry.tier == "community" || (!entry.downloadUrl.startsWith("https://cdn.modrinth.com/"))) {
            entry.tier = "community";
            communityMods.append(entry);
        }

        m_manifestMods.append(entry);
    }

    // Auto-ensure Fabric API if loader is Fabric with mods
    if (m_loader == "fabric" && !m_manifestMods.isEmpty()) {
        bool hasFabricApi = false;
        for (const auto& mod : m_manifestMods) {
            if (mod.fileName.toLower().contains("fabric-api") || mod.projectId == "fabric-api" || mod.projectId == "P7dR8mAc") {
                hasFabricApi = true;
                break;
            }
        }
        if (!hasFabricApi) {
            qDebug() << "[SVLModSync] Server uses Fabric with mods but no Fabric API in manifest. Adding Fabric API dependency...";
            SVLModEntry fabricApi;
            fabricApi.projectId = "P7dR8mAc";
            fabricApi.fileName = QString("fabric-api-0.102.0+%1.jar").arg(m_mcVersion);
            fabricApi.tier = "official";
            fabricApi.downloadUrl = QString("https://cdn.modrinth.com/data/P7dR8mAc/versions/0.102.0+%1/fabric-api-0.102.0+%1.jar").arg(m_mcVersion);
            m_manifestMods.prepend(fabricApi);
        }
    }

    // Security & Quarantine Check
    if (!m_verified && !communityMods.isEmpty()) {
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
    QString baseDir = APPLICATION->instances()->primaryDir();
    if (baseDir.isEmpty()) {
        baseDir = FS::PathCombine(APPLICATION->dataRoot(), "instances");
    }
    m_instance = APPLICATION->instances()->getInstanceById(m_serverKey);
    QString instanceDir = m_instance ? m_instance->instanceRoot() : FS::PathCombine(baseDir, m_serverKey);
    FS::ensureFolderPathExists(instanceDir);

    // 1. instance.cfg
    QString cfgPath = FS::PathCombine(instanceDir, "instance.cfg");
    if (!QFile::exists(cfgPath)) {
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
    }

    // 2. Build target mmc-pack.json
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

    QString targetLoaderUid;
    QString targetLoaderName;
    QString targetLoaderVersion = loaderVersion;

    if (loader == "fabric") {
        targetLoaderUid = "net.fabricmc.fabric-loader";
        targetLoaderName = "Fabric Loader";
        if (targetLoaderVersion.isEmpty()) targetLoaderVersion = "0.16.9";
    } else if (loader == "neoforge") {
        targetLoaderUid = "net.neoforged";
        targetLoaderName = "NeoForge";
        if (targetLoaderVersion.isEmpty()) targetLoaderVersion = "21.1.70";
    } else if (loader == "forge") {
        targetLoaderUid = "net.minecraftforge";
        targetLoaderName = "Forge";
        if (targetLoaderVersion.isEmpty()) targetLoaderVersion = "52.1.16";
    }

    if (!targetLoaderUid.isEmpty()) {
        QJsonObject loaderComp;
        loaderComp["cachedName"] = targetLoaderName;
        loaderComp["cachedVersion"] = targetLoaderVersion;
        loaderComp["uid"] = targetLoaderUid;
        loaderComp["version"] = targetLoaderVersion;
        components.append(loaderComp);
    }

    packObj["components"] = components;

    QByteArray newPackData = QJsonDocument(packObj).toJson(QJsonDocument::Indented);

    bool needsReload = false;
    QByteArray currentPackData;
    QFile existingPackFile(packPath);
    if (existingPackFile.open(QIODevice::ReadOnly)) {
        currentPackData = existingPackFile.readAll();
        existingPackFile.close();
    }

    if (currentPackData.trimmed() != newPackData.trimmed()) {
        qDebug() << "[SVLModSync] Updating mmc-pack.json to match target server: mc=" << mcVersion
                 << ", loader=" << targetLoaderUid << ":" << targetLoaderVersion;
        if (existingPackFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            existingPackFile.write(newPackData);
            existingPackFile.close();
            needsReload = true;
        }
    }

    // Ensure minecraft/mods directory exists
    QString modsFolder = FS::PathCombine(instanceDir, ".minecraft", "mods");
    FS::ensureFolderPathExists(modsFolder);

    if (needsReload || !m_instance) {
        APPLICATION->instances()->loadList();
        m_instance = APPLICATION->instances()->getInstanceById(m_serverKey);
        if (m_instance && m_instance->getPackProfile()) {
            m_instance->getPackProfile()->reload(Net::Mode::Offline);
            m_instance->getPackProfile()->invalidateLaunchProfile();
        }
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

    setStatus(tr("Downloading %1 mod(s)...").arg(m_modsToDownload.size()));
    setProgress(0, 100);

    m_netJob = makeShared<NetJob>(tr("Downloading mods for %1").arg(m_serverName), APPLICATION->network());

    for (const auto& mod : m_modsToDownload) {
        QString targetPath = FS::PathCombine(m_modsDirPath, mod.fileName);
        auto req = Net::NetRequest::makeFile(QUrl(mod.downloadUrl), targetPath);
        if (!mod.sha256.isEmpty()) {
            req->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, mod.sha256));
        }
        m_netJob->addNetAction(req);
    }

    // Use propagateFromOther to cleanly forward all signals (status, details,
    // progress, stepProgress) from the NetJob to this task and onward to the
    // ProgressDialog. This avoids the issue where NetJob::updateState() emits
    // raw progress(0, totalTasks) that competes with byte-level progress updates,
    // and overwrites the download status text with "Executing N task(s)...".
    propagateFromOther(m_netJob.get());

    connect(m_netJob.get(), &NetJob::succeeded, this, &SVLModSyncTask::onDownloadsSucceeded);
    connect(m_netJob.get(), &NetJob::failed, this, &SVLModSyncTask::onDownloadsFailed);
    connect(m_netJob.get(), &NetJob::aborted, this, &SVLModSyncTask::emitAborted);

    QMetaObject::invokeMethod(m_netJob.get(), &NetJob::start, Qt::QueuedConnection);
}

void SVLModSyncTask::onDownloadsSucceeded()
{
    qDebug() << "[SVLModSync] All mod downloads completed and verified successfully.";
    finalizeAndLaunch();
}

void SVLModSyncTask::onDownloadsFailed(const QString& reason)
{
    qWarning() << "[SVLModSync] Mod download job failed:" << reason;
    emitFailed(tr("Mod download failed: %1").arg(reason));
}

void SVLModSyncTask::ensureServerInServersDat()
{
    if (!m_instance) {
        return;
    }
    QString gameRoot = m_instance->gameRoot();
    QString serversDatPath = FS::PathCombine(gameRoot, "servers.dat");
    QString targetAddress = (m_serverPort == 25565) ? m_serverIp : QString("%1:%2").arg(m_serverIp).arg(m_serverPort);

    try {
        std::unique_ptr<nbt::tag_compound> rootCompound;
        if (QFile::exists(serversDatPath)) {
            QByteArray input = FS::read(serversDatPath);
            if (!input.isEmpty()) {
                std::istringstream stream(std::string(input.constData(), input.size()));
                auto pair = nbt::io::read_compound(stream);
                if (pair.first.empty() && pair.second) {
                    rootCompound = std::move(pair.second);
                }
            }
        }

        if (!rootCompound) {
            rootCompound = std::make_unique<nbt::tag_compound>();
        }

        nbt::tag_list* serversList = nullptr;
        if (rootCompound->has_key("servers", nbt::tag_type::List)) {
            serversList = &(*rootCompound)["servers"].as<nbt::tag_list>();
        } else {
            rootCompound->insert("servers", nbt::tag_list(nbt::tag_type::Compound));
            serversList = &(*rootCompound)["servers"].as<nbt::tag_list>();
        }

        bool found = false;
        for (auto& tagVal : *serversList) {
            if (tagVal.get_type() == nbt::tag_type::Compound) {
                auto& srvTag = tagVal.as<nbt::tag_compound>();
                if (srvTag.has_key("ip", nbt::tag_type::String)) {
                    std::string ipStr(srvTag["ip"]);
                    QString existingIp = QString::fromUtf8(ipStr.c_str());
                    if (existingIp.compare(targetAddress, Qt::CaseInsensitive) == 0 ||
                        existingIp.compare(m_serverIp, Qt::CaseInsensitive) == 0) {
                        srvTag.insert("name", m_serverName.toUtf8().toStdString());
                        srvTag.insert("ip", targetAddress.toUtf8().toStdString());
                        srvTag.insert("acceptTextures", nbt::tag_byte(1));
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!found) {
            nbt::tag_compound newServer;
            newServer.insert("name", m_serverName.toUtf8().toStdString());
            newServer.insert("ip", targetAddress.toUtf8().toStdString());
            newServer.insert("acceptTextures", nbt::tag_byte(1));
            serversList->push_back(std::move(newServer));
        }

        if (FS::ensureFilePathExists(serversDatPath)) {
            std::ostringstream s;
            nbt::io::write_tag("", *rootCompound, s);
            QByteArray outBytes(s.str().data(), static_cast<int>(s.str().size()));
            FS::write(serversDatPath, outBytes);
            qDebug() << "[SVLModSync] Successfully auto-populated servers.dat with" << m_serverName << "(" << targetAddress << ")";
        }
    } catch (const std::exception& e) {
        qWarning() << "[SVLModSync] Error writing servers.dat:" << e.what();
    } catch (...) {
        qWarning() << "[SVLModSync] Unknown error writing servers.dat.";
    }
}

void SVLModSyncTask::finalizeAndLaunch()
{
    setStatus(tr("Synchronization complete. Ready to launch."));
    setProgress(100, 100);

    ensureServerInServersDat();

    emit readyToLaunch(m_instance, m_serverIp, m_serverPort);
    emitSucceeded();
}
