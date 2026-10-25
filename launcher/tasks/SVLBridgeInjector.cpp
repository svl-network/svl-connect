#include "SVLBridgeInjector.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "FileSystem.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDebug>

SVLBridgeInjector::SVLBridgeInjector(QObject* parent) : QObject(parent) {}

ModLoaderType SVLBridgeInjector::detectLoader(const MinecraftInstance* instance) {
    if (!instance) return ModLoaderType::Vanilla;

    auto components = instance->getPackProfile();
    if (!components) return ModLoaderType::Vanilla;

    if (components->getComponent("net.fabricmc.fabric-loader")) {
        return ModLoaderType::Fabric;
    }
    if (components->getComponent("org.quiltmc.quilt-loader")) {
        return ModLoaderType::Quilt;
    }
    if (components->getComponent("net.neoforged")) {
        return ModLoaderType::NeoForge;
    }
    if (components->getComponent("net.minecraftforge")) {
        return ModLoaderType::Forge;
    }
    return ModLoaderType::Vanilla;
}

QString SVLBridgeInjector::loaderToString(ModLoaderType loader) {
    switch (loader) {
        case ModLoaderType::Fabric: return "Fabric";
        case ModLoaderType::Quilt: return "Quilt";
        case ModLoaderType::NeoForge: return "NeoForge";
        case ModLoaderType::Forge: return "Forge";
        case ModLoaderType::PaperServer: return "Paper";
        case ModLoaderType::Vanilla:
        default: return "Vanilla";
    }
}

QString SVLBridgeInjector::resolveBridgeJarPath(ModLoaderType loader, const QString& mcVersion) {
    // 1. Candidate search directories:
    // a. Bundled application jars
    // b. Repository releases path (for dev/build environments)
    QStringList searchDirs = {
        QDir(QCoreApplication::applicationDirPath()).filePath("jars/bridge"),
        QDir(QCoreApplication::applicationDirPath()).filePath("jars"),
        "c:/svl-network/svl-bridge/releases"
    };

    QString targetFileName;
    switch (loader) {
        case ModLoaderType::Fabric:
        case ModLoaderType::Quilt:
            targetFileName = "svl-bridge-fabric-1.0.0.jar";
            break;

        case ModLoaderType::NeoForge:
            if (mcVersion.startsWith("1.20.4")) {
                targetFileName = "svl-bridge-neoforge-1.20.4-1.0.0.jar";
            } else {
                targetFileName = "svl-bridge-neoforge-1.0.0.jar";
            }
            break;

        case ModLoaderType::Forge:
            if (mcVersion.startsWith("1.20.1")) {
                targetFileName = "svl-bridge-forge-1.20.1-1.0.0.jar";
            } else {
                targetFileName = "svl-bridge-forge-1.0.0.jar";
            }
            break;

        case ModLoaderType::PaperServer:
            targetFileName = "svl-bridge-paper-1.0.0.jar";
            break;

        case ModLoaderType::Vanilla:
        default:
            return QString();
    }

    for (const QString& dir : searchDirs) {
        QString candidate = QDir(dir).filePath(targetFileName);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

QString SVLBridgeInjector::generateFriendlyShareCode() {
    static const QStringList adjectives = {
        "swift", "shadow", "mystic", "cosmic", "solar", "lunar", "crystal", "frost",
        "ember", "golden", "silent", "ancient", "storm", "valiant", "blazing", "azure",
        "emerald", "iron", "radiant", "stellar", "noble", "vortex", "astral", "wild"
    };
    static const QStringList nouns = {
        "realm", "haven", "dragon", "citadel", "sanctum", "outpost", "summit",
        "bastion", "frontier", "peak", "oasis", "keep", "island", "forest", "nexus",
        "canyon", "spire", "grove", "garrison", "refuge", "valley"
    };

    int adjIdx = QRandomGenerator::global()->bounded(adjectives.size());
    int nounIdx = QRandomGenerator::global()->bounded(nouns.size());
    int num = QRandomGenerator::global()->bounded(100, 999);

    return QString("%1-%2-%3").arg(adjectives[adjIdx], nouns[nounIdx], QString::number(num));
}

bool SVLBridgeInjector::injectBridge(MinecraftInstance* instance, const BridgeConfigOptions& options, QString* errorOut) {
    if (!instance) {
        if (errorOut) *errorOut = "Instance pointer is null.";
        return false;
    }

    QString mcVersion = instance->getPackProfile() ? instance->getPackProfile()->getComponentVersion("net.minecraft") : QString();
    if (mcVersion.isEmpty()) {
        mcVersion = "1.21.1";
    }
    const ModLoaderType loader = detectLoader(instance);

    if (loader == ModLoaderType::Vanilla) {
        if (errorOut) *errorOut = "Multiplayer hosting tunnel requires a modloader (Fabric, Forge, or NeoForge) installed on the instance.";
        return false;
    }

    const QString sourceJar = resolveBridgeJarPath(loader, mcVersion);
    if (sourceJar.isEmpty() || !QFile::exists(sourceJar)) {
        if (errorOut) {
            *errorOut = QString("Could not find bundled Sunveil Bridge binary for %1 (%2).").arg(loaderToString(loader), mcVersion);
        }
        return false;
    }

    // 1. Ensure mods folder exists and copy bridge jar
    const QString modsDir = instance->modsRoot();
    FS::ensureFolderPathExists(modsDir);

    const QString targetJar = QDir(modsDir).filePath(QFileInfo(sourceJar).fileName());
    if (QFile::exists(targetJar)) {
        QFile::remove(targetJar); // overwrite with latest
    }

    if (!QFile::copy(sourceJar, targetJar)) {
        if (errorOut) *errorOut = QString("Failed to copy bridge binary to '%1'.").arg(targetJar);
        return false;
    }

    // 2. Generate and write config/svl-bridge.json
    const QString configDir = instance->instanceConfigFolder();
    FS::ensureFolderPathExists(configDir);

    QJsonObject root;
    root["masterApiUrl"] = options.masterApiUrl;
    root["masterApiToken"] = options.masterToken;
    root["serverKey"] = options.serverKey;
    root["serverName"] = options.serverName;
    root["publicIp"] = "auto";
    root["publicPort"] = 25565;
    root["localServerPort"] = options.localServerPort;
    root["heartbeatIntervalSeconds"] = 15;
    root["tunnelEnabled"] = options.tunnelEnabled;
    root["autoUpdateEnabled"] = options.autoUpdateEnabled;

    const QString configPath = QDir(configDir).filePath("svl-bridge.json");
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut) *errorOut = QString("Failed to write bridge config to '%1'.").arg(configPath);
        return false;
    }

    configFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    configFile.close();

    qDebug() << "[SVLBridgeInjector] Successfully injected bridge for instance:" << instance->name()
             << "Loader:" << loaderToString(loader)
             << "ServerKey:" << options.serverKey;
    return true;
}

bool SVLBridgeInjector::removeBridge(MinecraftInstance* instance) {
    if (!instance) return false;
    QDir modsDir(instance->modsRoot());
    QStringList bridgeFiles = modsDir.entryList(QStringList() << "*svl-bridge*.jar", QDir::Files);
    for (const QString& file : bridgeFiles) {
        modsDir.remove(file);
    }
    return true;
}
