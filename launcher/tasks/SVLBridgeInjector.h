#pragma once

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <memory>

class MinecraftInstance;

enum class ModLoaderType {
    Vanilla,
    Fabric,
    Forge,
    NeoForge,
    Quilt,
    PaperServer
};

struct BridgeConfigOptions {
    QString serverKey;
    QString masterApiUrl = "https://realms.sunveil.net/api/v1/heartbeat";
    QString masterToken;
    int localServerPort = 25565;
    bool tunnelEnabled = true;
    bool autoUpdateEnabled = true;
    QString serverName = "Sunveil Hosted World";
};

class SVLBridgeInjector : public QObject {
    Q_OBJECT

public:
    explicit SVLBridgeInjector(QObject* parent = nullptr);
    ~SVLBridgeInjector() override = default;

    // Detect loader from instance metadata and installed components
    static ModLoaderType detectLoader(const MinecraftInstance* instance);
    static QString loaderToString(ModLoaderType loader);

    // Resolves appropriate bundled bridge jar based on loader and MC version
    static QString resolveBridgeJarPath(ModLoaderType loader, const QString& mcVersion);

    // Injects the bridge mod jar and writes svl-bridge.json config
    static bool injectBridge(MinecraftInstance* instance, const BridgeConfigOptions& options, QString* errorOut = nullptr);

    // Removes the bridge mod if multiplayer hosting session is closed
    static bool removeBridge(MinecraftInstance* instance);

    // Generates a random friendly server key (e.g. "swift-dragon-482")
    static QString generateFriendlyShareCode();
};
