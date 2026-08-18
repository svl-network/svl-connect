#pragma once

#include <QString>
#include <QByteArray>
#include <QSysInfo>
#include <QCryptographicHash>
#include <QDateTime>
#include <QNetworkRequest>

namespace SVLSecurity {

inline const char* clientSecret() {
    return "svl_prod_sec_99a8b7c6d5";
}

// Generate a hashed HWID to ensure privacy while maintaining uniqueness
inline QString generateHWID() {
    static QString cachedHWID;
    if (!cachedHWID.isEmpty()) {
        return cachedHWID;
    }

    QByteArray machineId = QSysInfo::machineUniqueId();
    if (machineId.isEmpty()) {
        machineId = "SVL_FALLBACK_ID_" + QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    }
    cachedHWID = QString(QCryptographicHash::hash(machineId, QCryptographicHash::Sha256).toHex());
    return cachedHWID;
}

// Inject Security Headers into every Master API request
inline void injectAuthHeaders(QNetworkRequest& request) {
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-svl-hwid", generateHWID().toUtf8());
    request.setRawHeader("x-svl-client-secret", clientSecret());
}

} // namespace SVLSecurity
