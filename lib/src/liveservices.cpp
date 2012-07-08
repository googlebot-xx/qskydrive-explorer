/*************************************************************************
 ** This file is part of QSkyDrive Explorer
 ** Copyright (C) 2012 Stanislav Ionascu <stanislav.ionascu@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/

#include "liveservices.h"

#include "restclient.h"
#include "skydriveservice.h"

#include <QNetworkReply>
#include <qjson/parser.h>

#include <QDebug>

class LiveServicesPrivate
{
public:
    LiveServicesPrivate() :
        pendingSignInReply(0),
        skyDriveService(0)
    {
    }

    ~LiveServicesPrivate()
    {
    }

    void _q_checkSignInReply()
    {
        Q_Q(LiveServices);
        QVariantMap reply = parser.parse(pendingSignInReply).toMap();
        accessToken = reply.value("access_token").toString();

        qDebug() << Q_FUNC_INFO << reply;

        pendingSignInReply->deleteLater();
        pendingSignInReply = 0;

//        qDebug() << Q_FUNC_INFO << accessToken;

        if (!accessToken.isEmpty())
            emit q->signInSucceded();
    }

private:
    Q_DECLARE_PUBLIC(LiveServices)
    LiveServices *q_ptr;

    QNetworkReply *pendingSignInReply;
    SkyDriveService *skyDriveService;

    QString refreshToken;

    QString accessToken;
    QJson::Parser parser;
};

LiveServices::LiveServices(const QString &refreshToken, QObject *parent) :
    QObject(parent), d_ptr(new LiveServicesPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->refreshToken = refreshToken;
}

LiveServices::~LiveServices()
{
    delete d_ptr;
}

SkyDriveService *LiveServices::skyDriveService()
{
    Q_D(LiveServices);
    if (!d->skyDriveService)
        d->skyDriveService = new SkyDriveService(this);
    return d->skyDriveService;
}

void LiveServices::signIn()
{
    Q_D(LiveServices);

    QUrl url;
    url.setHost("login.live.com");
    url.setScheme("https");
    url.setPath("/oauth20_token.srf");

    QUrl queryItems;
    queryItems.addQueryItem("client_id", "000000004C0C2510");
    queryItems.addQueryItem("redirect_uri", "https://login.live.com/oauth20_desktop.srf");
    queryItems.addQueryItem("grant_type", "refresh_token");
    queryItems.addQueryItem("refresh_token", d->refreshToken);

    d->pendingSignInReply = RestClient::instance()->post(url, queryItems.encodedQuery());
    connect(d->pendingSignInReply, SIGNAL(finished()), this, SLOT(_q_checkSignInReply()));
}

const QString &LiveServices::accessToken() const
{
    Q_D(const LiveServices);
    return d->accessToken;
}

#include "moc_liveservices.cpp"
