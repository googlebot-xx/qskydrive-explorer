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

#include "restclient.h"

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>

class RestClientPrivate
{
public:
    RestClientPrivate() :
        q_ptr(0)
    {
        networkManager = new QNetworkAccessManager;
    }

    ~RestClientPrivate()
    {
        delete networkManager;
    }

    void connectToNetworkManager()
    {
        Q_Q(RestClient);
        q->connect(networkManager, SIGNAL(finished(QNetworkReply*)), q, SIGNAL(requestFinished(QNetworkReply*)));
    }

private:
    Q_DECLARE_PUBLIC(RestClient)
    RestClient *q_ptr;

    QNetworkAccessManager *networkManager;

    static RestClient *instance;
};
RestClient *RestClientPrivate::instance = 0;

RestClient::RestClient(QObject *parent) :
    QObject(parent), d_ptr(new RestClientPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->connectToNetworkManager();
}

RestClient::~RestClient()
{
    delete d_ptr;
}

QNetworkReply *RestClient::get(const QUrl &url)
{
    Q_D(RestClient);
    qDebug() << Q_FUNC_INFO << url;
    QNetworkRequest request;
    request.setUrl(url);
    return d->networkManager->get(request);
}

QNetworkReply *RestClient::post(const QUrl &url, const QByteArray &data)
{
    Q_D(RestClient);
    qDebug() << Q_FUNC_INFO << url << data;
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setUrl(url);
    return d->networkManager->post(request, data);
}

RestClient *RestClient::instance()
{
    if (!RestClientPrivate::instance)
        RestClientPrivate::instance = new RestClient;
    return RestClientPrivate::instance;
}

void RestClient::destroy()
{
    delete RestClientPrivate::instance;
    RestClientPrivate::instance = 0;
}
