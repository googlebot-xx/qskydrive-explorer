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

#include "authorizedialog.h"

#include "settings.h"
#include "restclient.h"

#include <QApplication>
#include <QWebView>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QNetworkReply>
#include <qjson/parser.h>

#include <QDebug>

AuthorizeDialog::AuthorizeDialog(QWidget *parent) :
    QDialog(parent), refreshTokenReply(0)
{
    setWindowTitle("Live Connect Authorization");
    requestAuthorizationCode();

    QRect r = frameGeometry();
    r.setSize(QSize(400, 600));
    r.moveCenter(qApp->desktop()->availableGeometry(0).center());
    setGeometry(r);
}

AuthorizeDialog::~AuthorizeDialog()
{
}

void AuthorizeDialog::requestAuthorizationCode()
{
    QWebView *view = new QWebView(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view);
    setLayout(layout);

    QUrl url;
    url.setHost("login.live.com");
    url.setScheme("https");
    url.setPath("/oauth20_authorize.srf");
    url.addQueryItem("client_id", "000000004C0C2510");
    url.addQueryItem("scope", "wl.signin wl.basic wl.skydrive wl.skydrive_update wl.offline_access");
    url.addQueryItem("response_type", "code");
    url.addQueryItem("redirect_uri", "https://login.live.com/oauth20_desktop.srf");
    url.addQueryItem("display", "touch");

    connect(view, SIGNAL(urlChanged(QUrl)), this, SLOT(verifyLiveConnectReply(QUrl)));
    view->load(url);
}

void AuthorizeDialog::verifyLiveConnectReply(const QUrl &url)
{
    qDebug() << Q_FUNC_INFO << url;
    if (!url.queryItemValue("code").isEmpty()) {
        qDebug() << Q_FUNC_INFO << url.queryItemValue("code");
        obtainRefreshToken(url.queryItemValue("code"));
    }
}

void AuthorizeDialog::obtainRefreshToken(const QString &authorizationCode)
{
    QUrl url;
    url.setHost("login.live.com");
    url.setScheme("https");
    url.setPath("/oauth20_token.srf");

    QUrl query;
    query.addQueryItem("client_id", "000000004C0C2510");
    query.addQueryItem("redirect_uri", "https://login.live.com/oauth20_desktop.srf");
    query.addQueryItem("code", authorizationCode);
    query.addQueryItem("grant_type", "authorization_code");

    refreshTokenReply = RestClient::instance()->post(url, query.encodedQuery());
    connect(refreshTokenReply, SIGNAL(readyRead()), this, SLOT(checkRefreshTokenResult()));
}

void AuthorizeDialog::checkRefreshTokenResult()
{
    QVariantMap result = QJson::Parser().parse(refreshTokenReply).toMap();

    if (result.contains("refresh_token") && result.contains("authentication_token")) {
        Settings::instance()->setValue("live/refreshToken", result.value("refresh_token").toString());
        Settings::instance()->setValue("live/authenticationToken", result.value("authentication_token").toString());
        accept();
    } else {
        reject();
    }

    refreshTokenReply->deleteLater();
    refreshTokenReply = 0;
}
