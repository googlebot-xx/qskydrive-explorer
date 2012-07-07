#include "skydriveservice.h"

#include "liveservices.h"
#include "restclient.h"

#include <QNetworkReply>
#include <qjson/parser.h>
#include <QDebug>

class SkyDriveServicePrivate
{
public:
    SkyDriveServicePrivate() :
        q_ptr(0),
        liveServices(0),
        pendingNetworkReply(0)
    {
    }

    ~SkyDriveServicePrivate()
    {
    }

    void _q_folderListReady()
    {
        Q_Q(SkyDriveService);

        QVariant data = parser.parse(pendingNetworkReply);
        emit q->folderListLoaded(data);

        pendingNetworkReply->deleteLater();
        pendingNetworkReply = 0;
    }

    void _q_folderCreateReady()
    {
        Q_Q(SkyDriveService);
        qDebug() << pendingNetworkReply->readAll();
        emit q->folderListUpdated();
    }

private:
    Q_DECLARE_PUBLIC(SkyDriveService)
    SkyDriveService *q_ptr;

    LiveServices *liveServices;
    QNetworkReply *pendingNetworkReply;

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
    d->pendingNetworkReply = RestClient::instance()->get(url);
    connect(d->pendingNetworkReply, SIGNAL(readyRead()), this, SLOT(_q_folderListReady()));
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
    data.insert("access_token", d->liveServices->accessToken());

    d->pendingNetworkReply = RestClient::instance()->post(url, data);
    connect(d->pendingNetworkReply, SIGNAL(readyRead()), this, SLOT(_q_folderCreateReady()));
}

void SkyDriveService::updateUserQuota()
{
    emit userQuotaUpdated(QVariant());
}

#include "moc_skydriveservice.cpp"
