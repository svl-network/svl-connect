// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "MSAStep.h"

#include <QAbstractOAuth2>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QOAuthHttpServerReplyHandler>
#include <QOAuthOobReplyHandler>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"

#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

bool isSchemeHandlerRegistered()
{
#ifdef Q_OS_LINUX
    QProcess process;
    process.start("xdg-mime", { "query", "default", "x-scheme-handler/" + BuildConfig.LAUNCHER_APP_BINARY_NAME });
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();

    return output.contains(APPLICATION->desktopFileName());

#elif defined(Q_OS_WIN)
    auto checkAndRegister = [](const QString& scheme) {
        QString regPath = QString("HKEY_CURRENT_USER\\Software\\Classes\\%1").arg(scheme);
        QSettings settings(regPath, QSettings::NativeFormat);
        const QString registeredRunCommand = settings.value("shell/open/command/.").toString().replace("\\", "/");
        QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        settings.setValue(".", QString("URL:%1 Protocol").arg(scheme));
        settings.setValue("URL Protocol", "");
        settings.setValue("shell/open/command/.", QString("\"%1\" \"%2\"").arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()), "%1"));
    };
    checkAndRegister("prismlauncher");
    checkAndRegister(BuildConfig.LAUNCHER_APP_BINARY_NAME);
#endif
    return true;
}

class DualOAuthReplyHandler final : public QOAuthHttpServerReplyHandler {
    Q_OBJECT

   public:
    explicit DualOAuthReplyHandler(quint16 port = 0, QObject* parent = nullptr)
        : QOAuthHttpServerReplyHandler(port, parent)
    {
        connect(APPLICATION, &Application::oauthReplyRecieved, this, &DualOAuthReplyHandler::handleCustomSchemeReply);
    }

    ~DualOAuthReplyHandler() override
    {
        disconnect(APPLICATION, &Application::oauthReplyRecieved, this, &DualOAuthReplyHandler::handleCustomSchemeReply);
    }

   private slots:
    void handleCustomSchemeReply(const QVariantMap& data)
    {
        callbackReceived(data);
    }

   protected:
    void networkReplyFinished(QNetworkReply* reply) override
    {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "OAuth2 request failed:" << reply->readAll();
        }

        QOAuthHttpServerReplyHandler::networkReplyFinished(reply);
    }
};

MSAStep::MSAStep(AccountData* data, bool silent) : AuthStep(data), m_silent(silent)
{
    m_clientId = APPLICATION->getMSAClientID();
    
    auto replyHandler = new DualOAuthReplyHandler(28443, this);
    if (!replyHandler->isListening()) {
        delete replyHandler;
        replyHandler = new DualOAuthReplyHandler(0, this);
    }

    replyHandler->setCallbackText(QString(R"XXX(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Sunveil Connect - Login Successful</title>
    <style>
        body { background-color: #121316; color: #f0f0f0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }
        .card { background: #1a1c23; border: 1px solid #333842; padding: 40px 60px; border-radius: 12px; text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        h1 { color: #22c55e; margin-bottom: 12px; font-size: 24px; font-weight: 600; }
        p { color: #aaa; margin: 0; font-size: 15px; }
    </style>
</head>
<body>
    <div class="card">
        <h1>✓ Erfolgreich angemeldet!</h1>
        <p>Du kannst diesen Browsertab jetzt schlie&szlig;en und zu Sunveil Connect zur&uuml;ckkehren.</p>
    </div>
</body>
</html>
)XXX"));
    m_oauth2.setReplyHandler(replyHandler);
    m_oauth2.setAuthorizationUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"));
    m_oauth2.setAccessTokenUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
    m_oauth2.setScope("XboxLive.SignIn XboxLive.offline_access");
    m_oauth2.setClientIdentifier(m_clientId);
    m_oauth2.setNetworkAccessManager(APPLICATION->network());

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::granted, this, [this] {
        m_data->msaClientID = m_oauth2.clientIdentifier();
        m_data->msaToken.issueInstant = QDateTime::currentDateTimeUtc();
        m_data->msaToken.notAfter = m_oauth2.expirationAt();
        m_data->msaToken.extra = m_oauth2.extraTokens();
        m_data->msaToken.refresh_token = m_oauth2.refreshToken();
        m_data->msaToken.token = m_oauth2.token();
        emit finished(AccountTaskState::STATE_WORKING, tr("Got MSA token"));
    });
    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &MSAStep::authorizeWithBrowser);
    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::requestFailed, this, [this, silent](const QAbstractOAuth2::Error err) {
        auto state = AccountTaskState::STATE_FAILED_HARD;
        if (m_oauth2.status() == QAbstractOAuth::Status::Granted || silent) {
            if (err == QAbstractOAuth2::Error::NetworkError) {
                state = AccountTaskState::STATE_OFFLINE;
            } else {
                state = AccountTaskState::STATE_FAILED_SOFT;
            }
        }
        auto message = tr("Microsoft user authentication failed.");
        if (silent) {
            message = tr("Failed to refresh token.");
        }
        qWarning() << message;
        emit finished(state, message);
    });
    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::error, this,
            [this](const QString& error, const QString& errorDescription, const QUrl& uri) {
                qWarning() << "Failed to login because" << error << errorDescription;
                emit finished(AccountTaskState::STATE_FAILED_HARD, errorDescription);
            });

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::extraTokensChanged, this,
            [this](const QVariantMap& tokens) { m_data->msaToken.extra = tokens; });

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::clientIdentifierChanged, this,
            [this](const QString& clientIdentifier) { m_data->msaClientID = clientIdentifier; });
}

QString MSAStep::describe()
{
    return tr("Logging in with Microsoft account.");
}

void MSAStep::perform()
{
    if (m_silent) {
        if (m_data->msaClientID != m_clientId) {
            emit finished(AccountTaskState::STATE_DISABLED,
                          tr("Microsoft user authentication failed - client identification has changed."));
            return;
        }
        if (m_data->msaToken.refresh_token.isEmpty()) {
            emit finished(AccountTaskState::STATE_DISABLED, tr("Microsoft user authentication failed - refresh token is empty."));
            return;
        }
        m_oauth2.setRefreshToken(m_data->msaToken.refresh_token);
        m_oauth2.refreshAccessToken();
    } else {
        m_oauth2.setModifyParametersFunction(
            [](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant>* map) { map->insert("prompt", "select_account"); });

        *m_data = AccountData();
        m_data->msaClientID = m_clientId;
        m_oauth2.grant();
    }
}

#include "MSAStep.moc"
