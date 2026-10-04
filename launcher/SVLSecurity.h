#pragma once

#include <QString>
#include <QByteArray>
#include <QSysInfo>
#include <QCryptographicHash>
#include <QDateTime>
#include <QNetworkRequest>

namespace SVLSecurity {

inline const char* clientSecret() {
#ifdef SVL_CLIENT_SECRET
    return SVL_CLIENT_SECRET;
#else
    return "";
#endif
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
    const char* sec = clientSecret();
    if (sec && sec[0] != '\0') {
        request.setRawHeader("x-svl-client-secret", sec);
    }
}

} // namespace SVLSecurity
