#include "SVLWardrobeManager.h"
#include "minecraft/MinecraftInstance.h"
#include "FileSystem.h"
#include "SVLSecurity.h"
#include "Application.h"
#include "settings/SettingsObject.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QNetworkRequest>
#include <QDebug>

SVLWardrobeManager::SVLWardrobeManager(QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent), m_nam(nam) {}

WardrobeProfile SVLWardrobeManager::loadLocalProfile() {
    WardrobeProfile profile;
    auto s = APPLICATION->settings();
    profile.activeCapeId = s->get("SVL_SelectedCape").toString();
    if (profile.activeCapeId.isEmpty()) {
        profile.activeCapeId = "solar_emerald";
    }
    profile.enableCapes = s->get("SVL_EnableCapes").toBool();
    profile.waveyCapes = s->get("SVL_WaveyCapes").toBool();
    profile.skinLayers3D = s->get("SVL_SkinLayers3D").toBool();
    profile.customCapeUrl = s->get("SVL_CustomCapeUrl").toString();
    return profile;
}

void SVLWardrobeManager::saveLocalProfile(const WardrobeProfile& profile) {
    auto s = APPLICATION->settings();
    s->set("SVL_SelectedCape", profile.activeCapeId);
    s->set("SVL_EnableCapes", profile.enableCapes);
    s->set("SVL_WaveyCapes", profile.waveyCapes);
    s->set("SVL_SkinLayers3D", profile.skinLayers3D);
    s->set("SVL_CustomCapeUrl", profile.customCapeUrl);
}

void SVLWardrobeManager::updateRemoteCape(const QString& uuid, const QString& capeIdentifier, const QByteArray& texturePngBytes) {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    QString cleanUuid = uuid.toLower().remove("-");
    if (cleanUuid.isEmpty()) cleanUuid = "demo_user";

    QUrl url(QString("%1/api/v1/capes/%2").arg(m_masterApiUrl, cleanUuid));
    QNetworkRequest request(url);
    SVLSecurity::injectAuthHeaders(request);

    QJsonObject payload;
    if (!texturePngBytes.isEmpty()) {
        payload["textureBase64"] = QString::fromLatin1(texturePngBytes.toBase64());
    } else {
        payload["cape"] = capeIdentifier;
    }

    m_currentReply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &SVLWardrobeManager::onCapeReplyFinished);
}

void SVLWardrobeManager::onCapeReplyFinished() {
    if (!m_currentReply) return;
    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    bool success = (reply->error() == QNetworkReply::NoError);
    emit capeUpdated(success, success ? "Cape updated successfully!" : reply->errorString());
}

bool SVLWardrobeManager::applyCosmeticsToInstance(MinecraftInstance* instance, const WardrobeProfile& profile) {
    if (!instance) return false;

    const QString configDir = instance->instanceConfigFolder();
    FS::ensureFolderPathExists(configDir);

    const QString targetConfigPath = QDir(configDir).filePath("sunveil-client.json");

    QJsonObject config;
    if (QFile::exists(targetConfigPath)) {
        QFile existing(targetConfigPath);
        if (existing.open(QIODevice::ReadOnly)) {
            config = QJsonDocument::fromJson(existing.readAll()).object();
            existing.close();
        }
    }

    config["enableCapes"] = profile.enableCapes;
    config["selectedCape"] = profile.activeCapeId;
    config["customCapeUrl"] = profile.customCapeUrl;
    config["waveyCapes"] = profile.waveyCapes;
    config["skinLayers3D"] = profile.skinLayers3D;
    config["chatHeads"] = profile.chatHeads;
    config["tabHeads"] = profile.tabHeads;

    QFile outFile(targetConfigPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[SVLWardrobeManager] Could not write sunveil-client.json to:" << targetConfigPath;
        return false;
    }

    outFile.write(QJsonDocument(config).toJson(QJsonDocument::Indented));
    outFile.close();

    qDebug() << "[SVLWardrobeManager] Injected wardrobe cosmetics into" << targetConfigPath
             << "Active Cape:" << profile.activeCapeId;
    return true;
}
