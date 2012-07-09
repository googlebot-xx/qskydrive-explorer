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

#include "skydriveservice.h"

#include "liveservices.h"
#include "restclient.h"

#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <qjson/parser.h>
#include <QDebug>

class SkyDriveServicePrivate
{
    enum Requests {
        InvalidRequest,
        FolderListRequest,
        FolderCreateRequest,
        UserQuotaRequest,
        RemoveRequest,
        UploadRequest
    };

public:
    SkyDriveServicePrivate() :
        q_ptr(0),
        liveServices(0)
    {
    }

    ~SkyDriveServicePrivate()
    {
    }

    void _q_folderListReady()
    {
        Q_Q(SkyDriveService);

        QNetworkReply *reply = pendingReplies[FolderListRequest];
        QVariant data = parser.parse(reply);
        emit q->folderListLoaded(data);

        reply->deleteLater();
        pendingReplies.remove(FolderListRequest);
    }

    void _q_folderCreateReady()
    {
        Q_Q(SkyDriveService);

        QNetworkReply *reply = pendingReplies[FolderCreateRequest];
        qDebug() << reply->readAll();
        emit q->folderListUpdated();

        reply->deleteLater();
        pendingReplies.remove(FolderCreateRequest);
    }

    void _q_userQuotaReady()
    {
        Q_Q(SkyDriveService);

        QNetworkReply *reply = pendingReplies[UserQuotaRequest];
        QVariant data = parser.parse(reply);
        emit q->userQuotaUpdated(data);

        qDebug() << data;

        reply->deleteLater();
        pendingReplies.remove(UserQuotaRequest);
    }

    void _q_processRemoveItemResult()
    {
        Q_Q(SkyDriveService);

        QNetworkReply *reply = pendingReplies[RemoveRequest];
        QVariant data = parser.parse(reply);
        emit q->itemRemoved();

        qDebug() << data;

        reply->deleteLater();
        pendingReplies.remove(RemoveRequest);
    }

    void _q_processUploadItemResult()
    {
        Q_Q(SkyDriveService);

        QNetworkReply *reply = pendingReplies[UploadRequest];
        QVariant data = parser.parse(reply);
        QFile *file = (QFile*)replyData[UploadRequest];
        emit q->itemUploaded();

        qDebug() << data;

        reply->deleteLater();
        file->deleteLater();
        pendingReplies.remove(UploadRequest);
    }

private:
    Q_DECLARE_PUBLIC(SkyDriveService)
    SkyDriveService *q_ptr;

    LiveServices *liveServices;
    QHash<Requests, QNetworkReply*> pendingReplies;
    QHash<Requests, QObject*> replyData;

    QJson::Parser parser;
};

SkyDriveService::SkyDriveService(LiveServices *parent) :
    QObject(parent), d_ptr(new SkyDriveServicePrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->liveServices = parent;
}

SkyDriveService::~SkyDriveService()
{
    delete d_ptr;
}

void SkyDriveService::loadFolderList(const QString &folderId)
{
    Q_D(SkyDriveService);
    QUrl url;
    url.setHost("apis.live.net");
    url.setScheme("https");
    if (folderId.isEmpty())
        url.setPath("/v5.0/me/skydrive/files");
    else
        url.setPath(QString("/v5.0/%1/files").arg(folderId));
    url.addQueryItem("access_token", d->liveServices->accessToken());

    QNetworkReply *reply = RestClient::instance()->get(url);
    d->pendingReplies[SkyDriveServicePrivate::FolderListRequest] = reply;
    connect(reply, SIGNAL(readyRead()), this, SLOT(_q_folderListReady()));
}

void SkyDriveService::createFolder(const QString &parentId, const QString &folderName)
{
    Q_D(SkyDriveService);
    QUrl url;
    url.setHost("apis.live.net");
    url.setScheme("https");
    if (parentId.isEmpty())
        url.setPath("/v5.0/me/skydrive");
    else
        url.setPath(QString("/v5.0/%1").arg(parentId));
    url.addQueryItem("access_token", d->liveServices->accessToken());

    QVariantMap data;
    data.insert("name", folderName);

    QNetworkReply *reply = RestClient::instance()->post(url, data);
    d->pendingReplies[SkyDriveServicePrivate::FolderCreateRequest] = reply;
    connect(reply, SIGNAL(readyRead()), this, SLOT(_q_folderCreateReady()));
}

void SkyDriveService::updateUserQuota()
{
    Q_D(SkyDriveService);
    QUrl url;
    url.setHost("apis.live.net");
    url.setScheme("https");
    url.setPath("/v5.0/me/skydrive/quota");
    url.addQueryItem("access_token", d->liveServices->accessToken());

    QNetworkReply *reply = RestClient::instance()->get(url);
    d->pendingReplies[SkyDriveServicePrivate::UserQuotaRequest] = reply;
    connect(reply, SIGNAL(readyRead()), this, SLOT(_q_userQuotaReady()));
}

void SkyDriveService::removeItem(const QString &itemId)
{
    Q_D(SkyDriveService);
    QUrl url;
    url.setHost("apis.live.net");
    url.setScheme("https");
    url.setPath(QString("/v5.0/%1").arg(itemId));
    url.addQueryItem("access_token", d->liveServices->accessToken());

    QNetworkReply *reply = RestClient::instance()->remove(url);
    d->pendingReplies[SkyDriveServicePrivate::RemoveRequest] = reply;
    connect(reply, SIGNAL(finished()), this, SLOT(_q_processRemoveItemResult()));
}

void SkyDriveService::uploadItem(const QString &parentId, const QString &path)
{
    Q_D(SkyDriveService);
    QUrl url;
    url.setHost("apis.live.net");
    url.setScheme("https");
    if (parentId.isEmpty())
        url.setPath(QString("/v5.0/me/skydrive/files/%1").arg(QFileInfo(path).fileName()));
    else
        url.setPath(QString("/v5.0/%1/files/%2").arg(parentId, QFileInfo(path).fileName()));
    url.addQueryItem("access_token", d->liveServices->accessToken());

    QFile *file = new QFile(path);
    if (!file->open(QFile::ReadOnly)) {
        qWarning() << "Could not open file for reading.";
        return;
    }
    QNetworkReply *reply = RestClient::instance()->put(url, file);
    d->pendingReplies[SkyDriveServicePrivate::UploadRequest] = reply;
    d->replyData[SkyDriveServicePrivate::UploadRequest] = file;
    connect(reply, SIGNAL(finished()), this, SLOT(_q_processUploadItemResult()));
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(itemUploadProgress(qint64, qint64)));
}

void SkyDriveService::cancelItemUpload()
{
    Q_D(SkyDriveService);
    QNetworkReply *reply = d->pendingReplies[SkyDriveServicePrivate::UploadRequest];
    QFile *file = (QFile*)d->replyData[SkyDriveServicePrivate::UploadRequest];
    if (reply) {
        reply->abort();

        reply->deleteLater();
        file->deleteLater();

        d->pendingReplies.remove(SkyDriveServicePrivate::UploadRequest);
        d->replyData.remove(SkyDriveServicePrivate::UploadRequest);
    }
}

#include "moc_skydriveservice.cpp"
