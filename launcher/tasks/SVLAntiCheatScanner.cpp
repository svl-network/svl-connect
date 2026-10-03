#include "tasks/SVLAntiCheatScanner.h"
#include "SVLSecurity.h"
#include "Application.h"
#include "settings/SettingsObject.h"
#include "archive/ArchiveReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

namespace {

static const QStringList KNOWN_CHEAT_SIGNATURES = {
    "meteor",
    "wurst",
    "liquidbounce",
    "aristois",
    "bleachhack",
    "inertia",
    "coffee",
    "ares",
    "sigma",
    "impact",
    "jex",
    "mathax",
    "future",
    "rusherhack",
    "kami",
    "lambda",
    "cornos",
    "atomic",
    "chlorine",
    "fdp",
    "vape",
    "augustus",
    "rise",
    "zeroday",
    "breeze",
    "doomsday",
    "baritone",
    "fabritone",
    "xray",
    "x-ray",
    "advanced-xray",
    "orebfuscator",
    "freecam",
    "autoclicker",
    "auto-clicker",
    "killaura",
    "seedcracker",
    "seed-cracker",
    "chestesp",
    "tracers",
    "flymod",
    "boatfly"
};

static const QStringList SOLID_BLOCK_TEXTURES = {
    "assets/minecraft/textures/block/stone.png",
    "assets/minecraft/textures/block/deepslate.png",
    "assets/minecraft/textures/block/netherrack.png",
    "assets/minecraft/textures/block/dirt.png",
    "assets/minecraft/textures/block/tuff.png",
    "assets/minecraft/textures/block/diorite.png",
    "assets/minecraft/textures/block/andesite.png",
    "assets/minecraft/textures/block/granite.png"
};

static bool isTextureTransparent(const QByteArray& pngData) {
    if (pngData.isEmpty()) {
        return true;
    }
    if (pngData.size() < 120) {
        // Transparent 1x1 stub PNG commonly used by simple X-Ray resource packs
        return true;
    }
    QImage img;
    if (img.loadFromData(pngData, "PNG")) {
        if (img.hasAlphaChannel()) {
            int w = qMin(img.width(), 64);
            int h = qMin(img.height(), 64);
            int transparentCount = 0;
            int total = w * h;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (img.pixelColor(x, y).alpha() < 240) {
                        transparentCount++;
                        if (transparentCount > (total * 0.05)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

} // namespace

bool SVLAntiCheatScanner::isAnticheatRequiredForServer(const QString& serverIp, const QStringList& disallowedMods)
{
    // Check if client-side inbuilt anti-cheat setting is active
    bool enabledInSettings = APPLICATION->settings()->get("InbuiltAntiCheatEnabled").toBool();
    if (!enabledInSettings) {
        return false;
    }

    QString ipLower = serverIp.trimmed().toLower();
    if (ipLower.contains("java.sunveil.net") || ipLower.contains("sunveil.net")) {
        return true;
    }
    if (!disallowedMods.isEmpty()) {
        return true;
    }
    return false;
}

SVLAntiCheatScanner::ScanResult SVLAntiCheatScanner::scanInstance(const QString& instanceGameRoot, const QStringList& disallowedMods)
{
    ScanResult result;
    result.clean = true;

    QCryptographicHash digestHash(QCryptographicHash::Sha256);

    // 1. Scan mods folder
    QString modsPath = QDir(instanceGameRoot).filePath("mods");
    QDir modsDir(modsPath);
    if (modsDir.exists()) {
        QStringList jarFiles = modsDir.entryList(QStringList() << "*.jar" << "*.JAR", QDir::Files);
        for (const QString& fileName : jarFiles) {
            QString fullPath = modsDir.filePath(fileName);
            QString lowerName = fileName.toLower();

            digestHash.addData(lowerName.toUtf8());

            bool isCheat = false;
            QString reason;

            // Check against known cheat keywords
            for (const QString& sig : KNOWN_CHEAT_SIGNATURES) {
                if (lowerName.contains(sig)) {
                    isCheat = true;
                    reason = QString("Known cheat signature match: '%1'").arg(sig);
                    break;
                }
            }

            // Check against server-side disallowed mods list
            if (!isCheat && !disallowedMods.isEmpty()) {
                for (const QString& disallowed : disallowedMods) {
                    if (lowerName.contains(disallowed.toLower())) {
                        isCheat = true;
                        reason = QString("Disallowed by server policy: '%1'").arg(disallowed);
                        break;
                    }
                }
            }

            // Deep content byte scanning inside jar file
            if (!isCheat) {
                QFile jarFile(fullPath);
                if (jarFile.open(QIODevice::ReadOnly)) {
                    // Read first 1MB of jar archive to check manifest / fabric.mod.json strings
                    QByteArray header = jarFile.read(1024 * 1024);
                    jarFile.close();

                    QString headerStr = QString::fromUtf8(header).toLower();
                    for (const QString& sig : KNOWN_CHEAT_SIGNATURES) {
                        QString idMatch = QString("\"id\": \"%1\"").arg(sig);
                        QString idMatch2 = QString("\"id\":\"%1\"").arg(sig);
                        if (headerStr.contains(idMatch) || headerStr.contains(idMatch2)) {
                            isCheat = true;
                            reason = QString("Internal mod id match: '%1'").arg(sig);
                            break;
                        }
                    }

                    if (!isCheat) {
                        if (headerStr.contains("cabaletta/baritone") || headerStr.contains("cabaletta.baritone")) {
                            isCheat = true;
                            reason = "Embedded Baritone automation framework detected.";
                        } else if (headerStr.contains("meteordevelopment")) {
                            isCheat = true;
                            reason = "Meteor Client exploit framework detected.";
                        } else if (headerStr.contains("net/wurstclient") || headerStr.contains("net.wurstclient")) {
                            isCheat = true;
                            reason = "Wurst Client exploit code detected.";
                        }
                    }
                }
            }

            if (isCheat) {
                result.clean = false;
                result.detectedCheatMods.append(QString("%1 (%2)").arg(fileName, reason));
                qWarning() << "[SVLAntiCheat] Flagged prohibited mod:" << fileName << reason;
            }
        }
    }

    // 2. Scan resourcepacks folder (X-Ray detection)
    QString rpPath = QDir(instanceGameRoot).filePath("resourcepacks");
    QDir rpDir(rpPath);
    if (rpDir.exists()) {
        QFileInfoList entries = rpDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& entry : entries) {
            QString packName = entry.fileName();
            QString lowerPackName = packName.toLower();

            digestHash.addData(lowerPackName.toUtf8());

            bool isXray = false;
            QString reason;

            // Name checks
            if (lowerPackName.contains("xray") || lowerPackName.contains("x-ray") ||
                lowerPackName.contains("x_ray") || lowerPackName.contains("ore-finder") ||
                lowerPackName.contains("diamond-finder") || lowerPackName.contains("transparent-block")) {
                isXray = true;
                reason = "Pack name indicates X-Ray exploitation.";
            }

            // Deep texture inspection
            if (!isXray && entry.isFile() && (entry.suffix().compare("zip", Qt::CaseInsensitive) == 0)) {
                try {
                    MMCZip::ArchiveReader reader(entry.absoluteFilePath());
                    for (const QString& texturePath : SOLID_BLOCK_TEXTURES) {
                        auto file = reader.goToFile(texturePath);
                        if (file) {
                            QByteArray pngBytes = file->readAll();
                            if (isTextureTransparent(pngBytes)) {
                                isXray = true;
                                reason = QString("Transparent texture detected on solid block: %1").arg(texturePath);
                                break;
                            }
                        }
                    }
                } catch (...) {
                    // Ignore archive read errors
                }
            } else if (!isXray && entry.isDir()) {
                QDir packSubDir(entry.absoluteFilePath());
                for (const QString& texturePath : SOLID_BLOCK_TEXTURES) {
                    QString fullTexPath = packSubDir.filePath(texturePath);
                    if (QFile::exists(fullTexPath)) {
                        QFile texFile(fullTexPath);
                        if (texFile.open(QIODevice::ReadOnly)) {
                            QByteArray pngBytes = texFile.readAll();
                            texFile.close();
                            if (isTextureTransparent(pngBytes)) {
                                isXray = true;
                                reason = QString("Transparent texture detected on solid block in directory pack: %1").arg(texturePath);
                                break;
                            }
                        }
                    }
                }
            }

            if (isXray) {
                result.clean = false;
                result.detectedXrayPacks.append(QString("%1 (%2)").arg(packName, reason));
                qWarning() << "[SVLAntiCheat] Flagged X-Ray resource pack:" << packName << reason;
            }
        }
    }

    // 3. Scan shaderpacks folder
    QString shaderPath = QDir(instanceGameRoot).filePath("shaderpacks");
    QDir shaderDir(shaderPath);
    if (shaderDir.exists()) {
        QStringList shaders = shaderDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& sName : shaders) {
            QString lowerShader = sName.toLower();
            digestHash.addData(lowerShader.toUtf8());
            if (lowerShader.contains("xray") || lowerShader.contains("x-ray") || lowerShader.contains("ore-glow")) {
                result.clean = false;
                result.detectedOtherViolations.append(QString("X-Ray shaderpack detected: %1").arg(sName));
                qWarning() << "[SVLAntiCheat] Flagged X-Ray shaderpack:" << sName;
            }
        }
    }

    result.scanDigest = QString(digestHash.result().toHex());

    if (!result.clean) {
        QStringList errors;
        if (!result.detectedCheatMods.isEmpty()) {
            errors.append(QString("Prohibited Mods (%1):\n• %2")
                              .arg(result.detectedCheatMods.size())
                              .arg(result.detectedCheatMods.join("\n• ")));
        }
        if (!result.detectedXrayPacks.isEmpty()) {
            errors.append(QString("X-Ray Resource Packs (%1):\n• %2")
                              .arg(result.detectedXrayPacks.size())
                              .arg(result.detectedXrayPacks.join("\n• ")));
        }
        if (!result.detectedOtherViolations.isEmpty()) {
            errors.append(QString("Other Prohibited Assets:\n• %1")
                              .arg(result.detectedOtherViolations.join("\n• ")));
        }
        result.errorMessage = errors.join("\n\n");
    }

    return result;
}

bool SVLAntiCheatScanner::attestCleanSession(const QString& masterApiBaseUrl,
                                            const QString& playerUuid,
                                            const QString& playerName,
                                            const QString& serverIp,
                                            const QString& scanDigest,
                                            QString& sessionTokenOut,
                                            QString& errorOut)
{
    QString base = masterApiBaseUrl.trimmed();
    if (base.isEmpty()) {
        base = "https://realms.sunveil.net";
    }

    QUrl attestUrl(base + "/api/v1/anticheat/attest");
    QNetworkRequest request(attestUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "SunveilConnect/1.0.0");
    request.setTransferTimeout(5000);

    SVLSecurity::injectAuthHeaders(request);

    QJsonObject bodyObj;
    bodyObj.insert("playerUuid", playerUuid);
    bodyObj.insert("playerName", playerName);
    bodyObj.insert("serverIp", serverIp);
    bodyObj.insert("clean", true);
    bodyObj.insert("scanDigest", scanDigest);
    bodyObj.insert("clientVersion", "SunveilConnect-1.0");

    QByteArray bodyBytes = QJsonDocument(bodyObj).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = APPLICATION->network()->post(request, bodyBytes);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(5000);

    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        errorOut = "Attestation request timed out.";
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        errorOut = QString("Master API responded with error: %1").arg(reply->errorString());
        reply->deleteLater();
        return false;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        errorOut = "Invalid JSON response from Master API.";
        return false;
    }

    QJsonObject obj = doc.object();
    if (obj.value("success").toBool() && obj.value("verified").toBool()) {
        sessionTokenOut = obj.value("sessionToken").toString();
        return true;
    } else {
        errorOut = obj.value("error").toString("Attestation rejected by server.");
        return false;
    }
}
