#pragma once

#include <QString>
#include <QStringList>
#include <QSet>
#include <QJsonObject>

/**
 * Inbuilt Anti-Cheat Scanner for Sunveil Connect.
 * 
 * Performs client-side integrity scanning of mods, resource packs, and shader packs
 * whenever connecting to a server that prohibits cheats (e.g. java.sunveil.net).
 * 
 * When 0 cheats and 0 X-Ray resource packs are found, an authenticated attestation
 * is registered with the Sunveil Master API so server-side anticheats (such as
 * TikTokSMPplus / SunveilSMPplus) automatically deactivate checks for this player,
 * eliminating false flagging completely.
 */
class SVLAntiCheatScanner {
public:
    struct ScanResult {
        bool clean = true;
        QStringList detectedCheatMods;
        QStringList detectedXrayPacks;
        QStringList detectedOtherViolations;
        QString scanDigest;
        QString errorMessage;
    };

    /**
     * Checks if the server enforces anti-cheat rules.
     * True for java.sunveil.net, *.sunveil.net, or if disallowedClientMods is specified.
     */
    static bool isAnticheatRequiredForServer(const QString& serverIp, const QStringList& disallowedMods);

    /**
     * Deeply scans the local instance game root for cheats and X-Ray packs.
     */
    static ScanResult scanInstance(const QString& instanceGameRoot, const QStringList& disallowedMods);

    /**
     * Submits an authenticated cryptographic attestation to the Sunveil Master API.
     */
    static bool attestCleanSession(const QString& masterApiBaseUrl,
                                   const QString& playerUuid,
                                   const QString& playerName,
                                   const QString& serverIp,
                                   const QString& scanDigest,
                                   QString& sessionTokenOut,
                                   QString& errorOut);
};
