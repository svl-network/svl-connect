#include "SVLModSyncTask.h"

#include <QApplication>
#include <QLabel>
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
#include "SVLSecurity.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ChecksumValidator.h"

#include "tasks/SVLAntiCheatScanner.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"

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
                               const QString& mcVersion,
                               const QString& loader,
                               QWidget* parentWidget)
    : Task(),
      m_masterApiBaseUrl(masterApiBaseUrl),
      m_serverKey(serverKey),
      m_serverName(serverName),
      m_serverIp(serverIp),
      m_serverPort(serverPort),
      m_parentWidget(parentWidget),
      m_mcVersion(mcVersion.isEmpty() ? "1.21.1" : mcVersion),
      m_loader(loader.isEmpty() ? "fabric" : loader)
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
    // Custom / Standalone servers directly prepare instance and connect without remote manifest
    if (m_serverKey.startsWith("custom_") || m_serverKey.startsWith("standalone_")) {
        setStatus(tr("Preparing standalone instance for %1...").arg(m_serverName));
        setProgress(30, 100);

        if (!prepareInstance(m_mcVersion.isEmpty() ? "1.21.1" : m_mcVersion, m_loader.isEmpty() ? "vanilla" : m_loader, "")) {
            emitFailed(tr("Failed to prepare Minecraft instance for %1.").arg(m_serverName));
            return;
        }

        setProgress(85, 100);
        ensureServerInServersDat();
        finalizeAndLaunch();
        return;
    }

    setStatus(tr("Connecting to Sunveil Master API..."));
    setProgress(0, 100);

    if (m_masterApiBaseUrl.isEmpty()) {
        m_masterApiBaseUrl = "https://realms.sunveil.net";
    }

    QUrl manifestUrl(m_masterApiBaseUrl + "/api/v1/servers/" + m_serverKey + "/manifest");
    QNetworkRequest request(manifestUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(10000);
    SVLSecurity::injectAuthHeaders(request);

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

    // Check if the server platform is a pure plugin server (Paper/Spigot/Velocity/etc)
    bool isPluginServer = (m_loader == "paper" || m_loader == "spigot" || m_loader == "bukkit" ||
                           m_loader == "purpur" || m_loader == "folia" || m_loader == "velocity" ||
                           m_loader == "bungee" || m_loader == "bungeecord" || m_loader == "waterfall");

    QJsonArray modsArray = obj.value("mods").toArray();
    for (const QJsonValue& val : modsArray) {
        QJsonObject modObj = val.toObject();
        SVLModEntry entry;
        entry.projectId = modObj.value("projectId").toString();
        
        // Strict filename sanitization: extract basename only to prevent directory traversal / Zip Slip
        QString rawFileName = modObj.value("fileName").toString();
        QString safeFileName = QFileInfo(rawFileName).fileName();
        if (safeFileName.isEmpty() || safeFileName.contains("..") || safeFileName.contains("/") || safeFileName.contains("\\")) {
            continue;
        }

        // Whitelist allowed file extensions for mods/packs/assets
        QString lowerExt = safeFileName.toLower();
        if (!lowerExt.endsWith(".jar") && !lowerExt.endsWith(".zip") && !lowerExt.endsWith(".json") &&
            !lowerExt.endsWith(".txt") && !lowerExt.endsWith(".png") && !lowerExt.endsWith(".toml") && !lowerExt.endsWith(".properties")) {
            qWarning() << "[SVLModSync] Prohibited file extension detected, skipping:" << safeFileName;
            continue;
        }

        entry.fileName = safeFileName;
        entry.sha256 = modObj.value("sha256").toString().toLower().trimmed();
        entry.downloadUrl = modObj.value("downloadUrl").toString();
        entry.tier = modObj.value("tier").toString("official").toLower();

        // Strict folder whitelisting: only allow known safe subdirectories inside gameRoot
        QString rawFolder = modObj.value("targetFolder").toString("mods").toLower().trimmed();
        if (rawFolder != "mods" && rawFolder != "shaderpacks" && rawFolder != "resourcepacks" && rawFolder != "config") {
            rawFolder = "mods";
        }
        entry.targetFolder = rawFolder;

        // If the server is a pure plugin server (Paper/Spigot), ignore server-only plugins
        if (isPluginServer) {
            qDebug() << "[SVLModSync] Excluding plugin from client instance on pure plugin server:" << entry.fileName;
            continue;
        }

        if (entry.tier == "community" || (!entry.downloadUrl.startsWith("https://cdn.modrinth.com/"))) {
            entry.tier = "community";
            communityMods.append(entry);
        }

        m_manifestMods.append(entry);
    }

    m_disallowedClientMods.clear();
    QJsonArray disallowedArray = obj.value("disallowedClientMods").toArray();
    for (const QJsonValue& val : disallowedArray) {
        QString modName = val.toString().trimmed().toLower();
        if (!modName.isEmpty()) {
            m_disallowedClientMods.append(modName);
        }
    }

    // Security & Quarantine Check
    if (!m_verified && !communityMods.isEmpty()) {
        QList<QWidget*> hiddenOverlays;
        for (QWidget* topWidget : QApplication::topLevelWidgets()) {
            if (topWidget && topWidget->inherits("QDialog") && topWidget->isVisible()) {
                if (topWidget->findChild<QLabel*>("loadingPrimaryStatus") != nullptr || topWidget->inherits("SVLLoadingOverlay")) {
                    topWidget->hide();
                    hiddenOverlays.append(topWidget);
                }
            }
        }

        SVLQuarantineDialog dialog(m_serverName, m_serverKey, communityMods, m_parentWidget ? m_parentWidget->window() : nullptr);
        int dialogResult = dialog.exec();

        for (QWidget* w : hiddenOverlays) {
            if (w) w->show();
        }

        if (dialogResult != QDialog::Accepted) {
            emitFailed(tr("Connection cancelled: Community mod quarantine was declined by the user."));
            return;
        }
    }

    setStatus(tr("Preparing local Minecraft instance..."));
    if (!prepareInstance(m_mcVersion, m_loader, m_loaderVersion)) {
        emitFailed(tr("Failed to create or configure local instance for server '%1'.").arg(m_serverKey));
        return;
    }

    injectInbuiltClientMods();

    performCleanSyncAndDownload();
}

void SVLModSyncTask::injectInbuiltClientMods()
{
    if (!m_instance) {
        return;
    }

    auto settings = APPLICATION->settings();
    bool altLook = settings->get("ClientMod_AltLook").toBool();
    bool freecam = settings->get("ClientMod_Freecam").toBool();
    bool minimap = settings->get("ClientMod_Minimap").toBool();
    bool itemPhysics = settings->get("ClientMod_ItemPhysics").toBool();
    bool fovZoom = settings->get("ClientMod_FovZoom").toBool();
    bool perf = settings->get("ClientMod_Performance").toBool();

    // Check server policy disallow list
    for (const QString& disallowed : m_disallowedClientMods) {
        QString d = disallowed.toLower().trimmed();
        if (d.contains("freecam")) freecam = false;
        if (d.contains("minimap") || d.contains("xaero")) minimap = false;
        if (d.contains("betterthirdperson") || d.contains("altlook") || d.contains("freelook")) altLook = false;
        if (d.contains("itemphysic")) itemPhysics = false;
        if (d.contains("zoom")) fovZoom = false;
    }

    // Check if the instance was already provisioned or already has mods installed
    QDir modsDir(m_modsDirPath);
    QStringList existingModFiles = modsDir.entryList(QStringList() << "*.jar" << "*.disabled" << "*.JAR" << "*.DISABLED", QDir::Files);
    bool alreadyProvisioned = m_instance->settings()->get("SVL_InbuiltModsProvisioned").toBool();
    if (!alreadyProvisioned && !existingModFiles.isEmpty()) {
        alreadyProvisioned = true;
        m_instance->settings()->set("SVL_InbuiltModsProvisioned", true);
    }

    QStringList userRemovedMods = m_instance->settings()->get("UserRemovedMods").toStringList();

    auto isModPresentInFolder = [&](const QString& keyword) {
        for (const QString& f : existingModFiles) {
            if (f.contains(keyword, Qt::CaseInsensitive)) {
                return true; // Exists as .jar or .disabled
            }
        }
        return false;
    };

    auto isExplicitlyRemovedByUser = [&](const QString& keyword, const QString& fileName) {
        for (const QString& r : userRemovedMods) {
            if (r.contains(keyword, Qt::CaseInsensitive) || fileName.compare(r, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        // If the instance has already been provisioned with mods, and this mod is not present at all,
        // it means the user deleted it from the instance!
        if (alreadyProvisioned && !isModPresentInFolder(keyword)) {
            return true;
        }
        return false;
    };

    auto hasModInManifest = [this](const QString& keyword) {
        for (const auto& m : m_manifestMods) {
            if (m.fileName.contains(keyword, Qt::CaseInsensitive) || m.projectId.contains(keyword, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    };

    bool is26 = m_mcVersion.startsWith("26.");
    bool needFabricApi = false;

    // ✦ OFFICIAL IN-HOUSE SUNVEIL CONNECT CLIENT MOD (Custom Capes, 360° Freelook, Smooth Zoom, Lunar HUD & Settings) ✦
    if (!hasModInManifest("sunveil-client") && !isExplicitlyRemovedByUser("sunveil-client", "sunveil-client-1.0.0.jar")) {
        QString bundledJar = QDir(QCoreApplication::applicationDirPath()).filePath("jars/sunveil-client-1.0.0.jar");
        QString targetPath = QDir(m_modsDirPath).filePath("sunveil-client-1.0.0.jar");

        if (QFile::exists(bundledJar) && !QFile::exists(targetPath)) {
            QDir(m_modsDirPath).mkpath(".");
            QFile::copy(bundledJar, targetPath);
            qDebug() << "[SVLModSync] Installed bundled official Sunveil Client mod:" << targetPath;
        } else if (!QFile::exists(targetPath)) {
            SVLModEntry entry;
            entry.projectId = "sunveil-client";
            entry.fileName = "sunveil-client-1.0.0.jar";
            entry.downloadUrl = "https://sunveil.net/api/client/sunveil-client-1.0.0.jar";
            entry.tier = "official";
            entry.targetFolder = "mods";
            m_manifestMods.append(entry);
        }
        needFabricApi = true;
    }

    if (perf) {
        if (!hasModInManifest("sodium") && !isExplicitlyRemovedByUser("sodium", is26 ? "sodium-fabric-0.9.2+mc26.1.2.jar" : "sodium-fabric-0.8.13+mc1.21.1.jar")) {
            SVLModEntry entry;
            entry.projectId = "sodium";
            if (is26) {
                entry.fileName = "sodium-fabric-0.9.2+mc26.1.2.jar";
                entry.sha256 = "dd19aaf6755788673941329cff855d89870dc9f69363c653c7f5378e799834f3";
                entry.downloadUrl = "https://cdn.modrinth.com/data/AANobbMI/versions/tZQ3jqnf/sodium-fabric-0.9.2%2Bmc26.1.2.jar";
            } else {
                entry.fileName = "sodium-fabric-0.8.13+mc1.21.1.jar";
                entry.sha256 = "3d43c14985a4deb19c654aad3393b454cf57e1ebcbba86c4ae6a98dfc60eca2d";
                entry.downloadUrl = "https://cdn.modrinth.com/data/AANobbMI/versions/SMxNOGZ6/sodium-fabric-0.8.13%2Bmc1.21.1.jar";
            }
            entry.tier = "official";
            entry.targetFolder = "mods";
            m_manifestMods.append(entry);
            needFabricApi = true;
        }
        if (!hasModInManifest("iris") && !isExplicitlyRemovedByUser("iris", is26 ? "iris-fabric-1.11.4+mc26.1.2.jar" : "iris-fabric-1.8.14-beta.1+mc1.21.1.jar")) {
            SVLModEntry entry;
            entry.projectId = "iris";
            if (is26) {
                entry.fileName = "iris-fabric-1.11.4+mc26.1.2.jar";
                entry.sha256 = "69bdb6899ba006cf91b16565e4445a7a4f331e39b790c81c99626814bf6e22df";
                entry.downloadUrl = "https://cdn.modrinth.com/data/YL57xq9U/versions/sZbVsl2Q/iris-fabric-1.11.4%2Bmc26.1.2.jar";
            } else {
                entry.fileName = "iris-fabric-1.8.14-beta.1+mc1.21.1.jar";
                entry.sha256 = "0ceb694040b4628bf3fa02286cfe5f0db833195669ab07f8caa4001359763eff";
                entry.downloadUrl = "https://cdn.modrinth.com/data/YL57xq9U/versions/bAo1Qhte/iris-fabric-1.8.14-beta.1%2Bmc1.21.1.jar";
            }
            entry.tier = "official";
            entry.targetFolder = "mods";
            m_manifestMods.append(entry);
            needFabricApi = true;
        }
    }

    if (needFabricApi && !hasModInManifest("fabric-api") && !isExplicitlyRemovedByUser("fabric-api", is26 ? "fabric-api-0.155.3+26.1.2.jar" : "fabric-api-0.116.17+1.21.1.jar")) {
        SVLModEntry entry;
        entry.projectId = "fabric-api";
        if (is26) {
            entry.fileName = "fabric-api-0.155.3+26.1.2.jar";
            entry.sha256 = "7fe7bea3dbb7b2e9ff998ab583130cd62fab22ab8a5229656ff14b2e648c91dd";
            entry.downloadUrl = "https://cdn.modrinth.com/data/P7dR8mSH/versions/3dM0X6ou/fabric-api-0.155.3%2B26.1.2.jar";
        } else {
            entry.fileName = "fabric-api-0.116.17+1.21.1.jar";
            entry.sha256 = "79ac44b40780acbd884b34c50be1e39af682847e5f5cb3b1fddeeaa768dce800";
            entry.downloadUrl = "https://cdn.modrinth.com/data/P7dR8mSH/versions/Mys3P7lK/fabric-api-0.116.17%2B1.21.1.jar";
        }
        entry.tier = "official";
        entry.targetFolder = "mods";
        m_manifestMods.append(entry);
    }

    if (!alreadyProvisioned) {
        m_instance->settings()->set("SVL_InbuiltModsProvisioned", true);
    }
}

bool SVLModSyncTask::prepareInstance(const QString& mcVersion, const QString& loader, const QString& loaderVersion)
{
    QString baseDir = APPLICATION->instances()->primaryDir();
    if (baseDir.isEmpty()) {
        baseDir = FS::PathCombine(APPLICATION->dataRoot(), "instances");
    }

    // Sanitize folder name cleanly
    QString cleanKey = m_serverKey;
    cleanKey.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "_");
    while (cleanKey.contains("__")) cleanKey.replace("__", "_");
    cleanKey.remove(QRegularExpression("^_+|_+$"));
    if (cleanKey.isEmpty()) cleanKey = "svl_instance";

    // 1. Try finding existing instance by serverKey ID
    m_instance = APPLICATION->instances()->getInstanceById(m_serverKey);
    // 2. Try finding by clean sanitized folder ID
    if (!m_instance) {
        m_instance = APPLICATION->instances()->getInstanceById(cleanKey);
    }
    // 3. Try finding by ManagedName (which stores serverKey)
    if (!m_instance) {
        m_instance = APPLICATION->instances()->getInstanceByManagedName(m_serverKey);
    }
    // 4. Try finding by matching display name
    if (!m_instance) {
        for (int i = 0; i < APPLICATION->instances()->count(); ++i) {
            auto* candidate = APPLICATION->instances()->at(i);
            if (candidate && candidate->name().compare(m_serverName, Qt::CaseInsensitive) == 0) {
                m_instance = candidate;
                break;
            }
        }
    }

    QString instanceDir = m_instance ? m_instance->instanceRoot() : FS::PathCombine(baseDir, cleanKey);
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
                                      "OverrideCommands=false\n"
                                      "OverrideJava=true\n"
                                      "IgnoreJavaCompatibility=true\n")
                                  .arg(m_serverName, m_serverKey);
            cfgFile.write(content.toUtf8());
            cfgFile.close();
        }
    } else {
        QFile cfgFile(cfgPath);
        if (cfgFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString content = QString::fromUtf8(cfgFile.readAll());
            bool modified = false;
            if (!content.contains("IgnoreJavaCompatibility=true")) {
                if (!content.endsWith("\n")) content += "\n";
                content += "OverrideJava=true\nIgnoreJavaCompatibility=true\n";
                modified = true;
            }
            if (!content.contains("ManagedName=")) {
                content += QString("ManagedName=%1\n").arg(m_serverKey);
                modified = true;
            }
            if (modified) {
                cfgFile.resize(0);
                cfgFile.write(content.toUtf8());
            }
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

    if (loader == "neoforge") {
        targetLoaderUid = "net.neoforged";
        targetLoaderName = "NeoForge";
        if (targetLoaderVersion.isEmpty() || !targetLoaderVersion.contains(".")) {
            if (mcVersion.startsWith("1.21.1")) targetLoaderVersion = "21.1.70";
            else if (mcVersion.startsWith("1.21")) targetLoaderVersion = "21.0.167";
            else if (mcVersion.startsWith("1.20.6")) targetLoaderVersion = "20.6.119";
            else if (mcVersion.startsWith("1.20.4")) targetLoaderVersion = "20.4.237";
            else if (mcVersion.startsWith("1.20.2")) targetLoaderVersion = "20.2.88";
            else if (mcVersion.startsWith("1.20.1")) targetLoaderVersion = "20.1.100";
            else targetLoaderVersion = "21.1.70";
        }
    } else if (loader == "forge") {
        targetLoaderUid = "net.minecraftforge";
        targetLoaderName = "Forge";
        if (targetLoaderVersion.isEmpty() || !targetLoaderVersion.contains(".")) {
            if (mcVersion.startsWith("1.21.1")) targetLoaderVersion = "52.1.16";
            else if (mcVersion.startsWith("1.20.4")) targetLoaderVersion = "49.0.38";
            else if (mcVersion.startsWith("1.20.1")) targetLoaderVersion = "47.2.20";
            else if (mcVersion.startsWith("1.19.4")) targetLoaderVersion = "45.1.0";
            else if (mcVersion.startsWith("1.18.2")) targetLoaderVersion = "40.2.14";
            else if (mcVersion.startsWith("1.16.5")) targetLoaderVersion = "36.2.39";
            else if (mcVersion.startsWith("1.12.2")) targetLoaderVersion = "14.23.5.2860";
            else targetLoaderVersion = "52.1.16";
        }
    } else {
        // Default to Fabric Loader (for Fabric servers, Paper/Spigot plugin servers, and client mod suite)
        targetLoaderUid = "net.fabricmc.fabric-loader";
        targetLoaderName = "Fabric Loader";
        if (targetLoaderVersion.isEmpty() || !targetLoaderVersion.startsWith("0.") || targetLoaderVersion < "0.19.5") {
            targetLoaderVersion = "0.19.5";
        }
    }

    if (targetLoaderUid == "net.fabricmc.fabric-loader") {
        QJsonObject intermediaryComp;
        intermediaryComp["cachedName"] = "Intermediary Mappings";
        QJsonArray reqs;
        QJsonObject req;
        req["equals"] = mcVersion;
        req["uid"] = "net.minecraft";
        reqs.append(req);
        intermediaryComp["cachedRequires"] = reqs;
        intermediaryComp["cachedVersion"] = mcVersion;
        intermediaryComp["dependencyOnly"] = true;
        intermediaryComp["uid"] = "net.fabricmc.intermediary";
        intermediaryComp["version"] = mcVersion;
        components.append(intermediaryComp);

        QJsonObject loaderComp;
        loaderComp["cachedName"] = targetLoaderName;
        QJsonArray loaderReqs;
        QJsonObject loaderReq;
        loaderReq["uid"] = "net.fabricmc.intermediary";
        loaderReqs.append(loaderReq);
        loaderComp["cachedRequires"] = loaderReqs;
        loaderComp["cachedVersion"] = targetLoaderVersion;
        loaderComp["uid"] = targetLoaderUid;
        loaderComp["version"] = targetLoaderVersion;
        components.append(loaderComp);
    } else if (!targetLoaderUid.isEmpty()) {
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

    if (m_instance->settings()) {
        m_instance->settings()->set("OverrideJava", true);
        m_instance->settings()->set("IgnoreJavaCompatibility", true);

        // Sunveil Performance & Zero-Lag Network Tuning
        m_instance->settings()->set("OverrideJavaArgs", true);
        QString optJvmArgs = "-XX:+UseG1GC -XX:+ParallelRefProcEnabled -XX:MaxGCPauseMillis=200 -XX:+UnlockExperimentalVMOptions -XX:+AlwaysPreTouch -XX:G1NewSizePercent=30 -XX:G1MaxNewSizePercent=40 -XX:G1ReservePercent=20 -XX:G1HeapWastePercent=5 -XX:G1MixedGCCountTarget=4 -XX:InitiatingHeapOccupancyPercent=15 -XX:G1MixedGCLiveThresholdPercent=90 -XX:G1RSetUpdatingPauseTimePercent=5 -XX:SurvivorRatio=32 -XX:+PerfDisableSharedMem -XX:MaxTenuringThreshold=1 -Dio.netty.allocator.type=pooled -Djava.net.preferIPv4Stack=true -Dsun.net.inetaddr.ttl=30 -Dnetworkaddress.cache.ttl=30";
        m_instance->settings()->set("JvmArgs", optJvmArgs);

        // Dynamic RAM auto-scaling (minimum 4GB heap to eliminate OutOfMemory and GC stutter)
        m_instance->settings()->set("OverrideMemory", true);
        int curMaxMem = m_instance->settings()->get("MaxMemAlloc").toInt();
        if (curMaxMem < 4096) {
            m_instance->settings()->set("MaxMemAlloc", 4096);
        }
        int curMinMem = m_instance->settings()->get("MinMemAlloc").toInt();
        if (curMinMem < 2048) {
            m_instance->settings()->set("MinMemAlloc", 2048);
        }
    }

    // Determine correct mods folder path using the instance's canonical mods directory
    FS::ensureFolderPathExists(m_instance->gameRoot());
    m_modsDirPath = m_instance->modsRoot();
    FS::ensureFolderPathExists(m_modsDirPath);

    // If an older sync created .minecraft/mods while gameRoot is minecraft/, migrate existing files
    QString legacyDotMcMods = FS::PathCombine(m_instance->instanceRoot(), ".minecraft", "mods");
    if (QDir(legacyDotMcMods).exists() && legacyDotMcMods != m_modsDirPath) {
        QDir legacyDir(legacyDotMcMods);
        for (const QString& file : legacyDir.entryList(QDir::Files)) {
            QString src = legacyDir.absoluteFilePath(file);
            QString dst = FS::PathCombine(m_modsDirPath, file);
            if (!QFile::exists(dst)) {
                QFile::rename(src, dst);
            } else {
                QFile::remove(src);
            }
        }
        legacyDir.removeRecursively();
    }
    return true;
}

void SVLModSyncTask::performCleanSyncAndDownload()
{
    setStatus(tr("Verifying local mods & shaderpacks..."));

    QString gameRoot = m_instance->gameRoot();
    QStringList syncFolders = { "mods", "shaderpacks", "resourcepacks" };

    QMap<QString, SVLModEntry> manifestBySha;
    QSet<QString> manifestFileNames;
    for (const auto& mod : m_manifestMods) {
        if (!mod.sha256.isEmpty()) {
            manifestBySha.insert(mod.sha256.toLower().trimmed(), mod);
        }
        if (!mod.fileName.isEmpty()) {
            manifestFileNames.insert(mod.fileName.toLower().trimmed());
        }
    }

    QSet<QString> localHashes;

    for (const QString& folderName : syncFolders) {
        QString folderPath = (folderName == "mods") ? m_modsDirPath : FS::PathCombine(gameRoot, folderName);
        FS::ensureFolderPathExists(folderPath);

        QDir dir(folderPath);
        QStringList fileFilters = (folderName == "mods") ? (QStringList() << "*.jar" << "*.JAR") : (QStringList() << "*.zip" << "*.ZIP" << "*.jar" << "*.JAR");
        QStringList localFiles = dir.entryList(fileFilters, QDir::Files);

        if (folderName == "mods") {
            // Keep user-disabled files disabled! Do not forcefully re-enable them.
            QStringList disabledModFiles = dir.entryList(QStringList() << "*.disabled" << "*.DISABLED", QDir::Files);
            for (const QString& dis : disabledModFiles) {
                QString base = dis;
                base.chop(QString(".disabled").length());
                localHashes.insert(base.toLower().trimmed());
                localHashes.insert(dis.toLower().trimmed());
            }
            localFiles = dir.entryList(fileFilters, QDir::Files);
        }

        for (const QString& localFile : localFiles) {
            QString fullPath = dir.absoluteFilePath(localFile);

            if (folderName == "mods") {
                auto settings = APPLICATION->settings();
                bool altLook = settings->get("ClientMod_AltLook").toBool();
                bool freecam = settings->get("ClientMod_Freecam").toBool();
                bool minimap = settings->get("ClientMod_Minimap").toBool();
                bool itemPhysics = settings->get("ClientMod_ItemPhysics").toBool();
                bool fovZoom = settings->get("ClientMod_FovZoom").toBool();
                bool perf = settings->get("ClientMod_Performance").toBool();

                QString lowerName = localFile.toLower();
                bool shouldDisable = false;
                if (!altLook && (lowerName.contains("betterthirdperson") || lowerName.contains("better-third-person"))) shouldDisable = true;
                if (!freecam && lowerName.contains("freecam")) shouldDisable = true;
                if (!minimap && (lowerName.contains("xaero") || lowerName.contains("minimap"))) shouldDisable = true;
                if (!itemPhysics && lowerName.contains("itemphysic")) shouldDisable = true;
                if (!fovZoom && lowerName.contains("zoomify")) shouldDisable = true;
                if (!perf && (lowerName.contains("sodium") || lowerName.contains("iris") || lowerName.contains("embeddium") || lowerName.contains("oculus"))) shouldDisable = true;

                if (!m_disallowedClientMods.isEmpty()) {
                    for (const QString& disallowed : m_disallowedClientMods) {
                        if (lowerName.contains(disallowed)) {
                            shouldDisable = true;
                            break;
                        }
                    }
                }

                if (shouldDisable) {
                    qDebug() << "[SVLModSync] Disabling client-side mod:" << localFile;
                    setStatus(tr("Disabling client mod '%1'...").arg(localFile));
                    QFile::rename(fullPath, fullPath + ".disabled");
                    continue;
                }
            }

            QFile file(fullPath);
            if (file.open(QIODevice::ReadOnly)) {
                QString hash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex().toLower().trimmed();
                file.close();

                bool isManifestMatch = manifestBySha.contains(hash);
                if (!isManifestMatch && manifestFileNames.contains(localFile.toLower().trimmed())) {
                    for (const auto& m : m_manifestMods) {
                        if (m.fileName.compare(localFile, Qt::CaseInsensitive) == 0 && (m.sha256.isEmpty() || m.sha256.toLower().trimmed() == hash)) {
                            isManifestMatch = true;
                            break;
                        }
                    }
                }

                if (!isManifestMatch) {
                    // Check if this local file is an outdated version of an incoming manifest mod
                    bool isOutdatedVersionOfManifestMod = false;
                    for (const auto& m : m_manifestMods) {
                        QString target = m.targetFolder.isEmpty() ? "mods" : m.targetFolder.toLower();
                        if (target == folderName) {
                            if (!m.projectId.isEmpty() && localFile.toLower().contains(m.projectId.toLower())) {
                                isOutdatedVersionOfManifestMod = true;
                                break;
                            }
                        }
                    }

                    if (isOutdatedVersionOfManifestMod) {
                        qDebug() << "[SVLModSync] Replacing outdated manifest file in" << folderName << ":" << localFile;
                        QFile::remove(fullPath);
                    } else {
                        // Preserve user-installed client mods, shaderpacks, and resourcepacks
                        qDebug() << "[SVLModSync] Preserving user-installed file in" << folderName << ":" << localFile;
                    }
                } else {
                    localHashes.insert(hash);
                    localHashes.insert(localFile.toLower().trimmed());
                }
            }
        }
    }

    // Determine missing mods & assets
    m_modsToDownload.clear();
    for (const auto& mod : m_manifestMods) {
        if (mod.downloadUrl.isEmpty()) {
            continue;
        }
        QString cleanSha = mod.sha256.toLower().trimmed();
        QString cleanName = mod.fileName.toLower().trimmed();
        if (!cleanSha.isEmpty()) {
            if (!localHashes.contains(cleanSha)) {
                m_modsToDownload.append(mod);
            }
        } else {
            if (!localHashes.contains(cleanName)) {
                m_modsToDownload.append(mod);
            }
        }
    }

    if (m_modsToDownload.isEmpty()) {
        qDebug() << "[SVLModSync] All mods and shaderpacks are up-to-date and cryptographically verified.";
        finalizeAndLaunch();
        return;
    }

    setStatus(tr("Downloading %1 mod(s) and asset(s)...").arg(m_modsToDownload.size()));
    setProgress(0, 100);

    m_netJob = makeShared<NetJob>(tr("Downloading assets for %1").arg(m_serverName), APPLICATION->network());

    for (const auto& mod : m_modsToDownload) {
        QString folder = mod.targetFolder.isEmpty() ? "mods" : mod.targetFolder.toLower();
        if (folder != "mods" && folder != "resourcepacks" && folder != "shaderpacks") {
            folder = "mods";
        }
        QString destFolder = (folder == "mods") ? m_modsDirPath : FS::PathCombine(gameRoot, folder);
        FS::ensureFolderPathExists(destFolder);

        QString safeBaseName = QFileInfo(mod.fileName).fileName().trimmed();
        safeBaseName.remove("..");
        safeBaseName.remove("/");
        safeBaseName.remove("\\");

        // Anti-Malware & Native Executable Guard
        QString lowerName = safeBaseName.toLower();
        if (!lowerName.endsWith(".jar") && !lowerName.endsWith(".zip")) {
            qWarning() << "[SVLModSync] Security violation: Non-JAR/ZIP download rejected:" << mod.fileName;
            continue;
        }

        const QStringList blockedExts = { ".exe", ".bat", ".cmd", ".ps1", ".vbs", ".dll", ".so", ".dylib", ".scr", ".msi", ".pif", ".hta", ".cpl", ".reg" };
        bool isBlocked = false;
        for (const QString& ext : blockedExts) {
            if (lowerName.contains(ext)) {
                isBlocked = true;
                break;
            }
        }
        if (isBlocked) {
            qWarning() << "[SVLModSync] Security violation: Dangerous payload extension detected in:" << mod.fileName;
            continue;
        }

        // Insecure Protocol Guard: Enforce HTTPS strictly
        if (!mod.downloadUrl.startsWith("https://", Qt::CaseInsensitive)) {
            qWarning() << "[SVLModSync] Security violation: Insecure HTTP download rejected for:" << mod.fileName;
            continue;
        }

        QString targetPath = FS::PathCombine(destFolder, safeBaseName);

        // Security assertion: target path must strictly reside within instance gameRoot
        QString cleanTarget = QDir::cleanPath(targetPath);
        QString cleanRoot = QDir::cleanPath(gameRoot);
        if (!cleanTarget.startsWith(cleanRoot)) {
            qWarning() << "[SVLModSync] Security assertion failed: path traversal attempt blocked for" << mod.fileName;
            continue;
        }

        // If the mod was disabled by the user (.disabled), do not re-download
        if (QFile::exists(targetPath + ".disabled") || QFile::exists(targetPath + ".DISABLED")) {
            qDebug() << "[SVLModSync] Skipping download of user-disabled mod:" << mod.fileName;
            continue;
        }

        // If the mod was explicitly removed by the user in Edit Instance, do not re-download
        if (m_instance && m_instance->settings()) {
            QStringList userRemoved = m_instance->settings()->get("UserRemovedMods").toStringList();
            bool removed = false;
            for (const QString& r : userRemoved) {
                if (mod.fileName.compare(r, Qt::CaseInsensitive) == 0 ||
                    (!mod.projectId.isEmpty() && r.contains(mod.projectId, Qt::CaseInsensitive)) ||
                    r.contains(QFileInfo(mod.fileName).baseName(), Qt::CaseInsensitive)) {
                    removed = true;
                    break;
                }
            }
            if (removed) {
                qDebug() << "[SVLModSync] Skipping download of user-removed mod:" << mod.fileName;
                continue;
            }
        }

        if (QFile::exists(targetPath)) {
            QFile::remove(targetPath);
        }

        QUrl url = QUrl::fromEncoded(mod.downloadUrl.toUtf8());
        if (!url.isValid() || url.scheme().isEmpty()) {
            url = QUrl(mod.downloadUrl);
        }

        auto req = Net::NetRequest::makeFile(url, targetPath);
        if (!mod.sha256.isEmpty()) {
            req->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, mod.sha256.toLower().trimmed()));
        }
        m_netJob->addNetAction(req);
    }

    if (m_netJob->size() == 0) {
        qDebug() << "[SVLModSync] All assets are up-to-date or intentionally excluded by user. Launching...";
        finalizeAndLaunch();
        return;
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
    if (m_instance && SVLAntiCheatScanner::isAnticheatRequiredForServer(m_serverIp, m_disallowedClientMods)) {
        setStatus(tr("Scanning instance with Inbuilt Anti-Cheat..."));
        auto scanResult = SVLAntiCheatScanner::scanInstance(m_instance->gameRoot(), m_disallowedClientMods);
        if (!scanResult.clean) {
            emitFailed(tr("Connection rejected: Server '%1' prohibits cheats and X-Ray packs.\n\n%2")
                           .arg(m_serverName, scanResult.errorMessage));
            return;
        }

        // Attest clean session to Master API
        auto account = APPLICATION->accounts()->defaultAccount();
        QString uuid = account ? account->profileId() : "";
        QString name = account ? account->profileName() : "";
        if (!uuid.isEmpty()) {
            setStatus(tr("Attesting clean client status to Sunveil Network..."));
            QString token, err;
            bool ok = SVLAntiCheatScanner::attestCleanSession(m_masterApiBaseUrl, uuid, name, m_serverIp, scanResult.scanDigest, token, err);
            if (ok) {
                qDebug() << "[SVLAntiCheat] Successfully registered clean attestation for" << name << "(" << uuid << ")";
            } else {
                qWarning() << "[SVLAntiCheat] Attestation notice:" << err;
            }
        }
    }

    setStatus(tr("Synchronization complete. Ready to launch."));
    setProgress(100, 100);

    ensureServerInServersDat();

    emit readyToLaunch(m_instance, m_serverIp, m_serverPort);
    emitSucceeded();
}
