#ifndef SKYDRIVESERVICE_H
#define SKYDRIVESERVICE_H

#include <QObject>
#include <QVariantMap>

class LiveServices;
class SkyDriveServicePrivate;

class SkyDriveService : public QObject
{
    Q_OBJECT
public:
    explicit SkyDriveService(LiveServices *parent = 0);
    virtual ~SkyDriveService();

signals:
    void folderListLoaded(const QVariant &data);

public slots:
    void loadFolderList(const QString &folderId = "");

private:
    Q_PRIVATE_SLOT(d_func(), void _q_folderListReady())

private:
    Q_DECLARE_PRIVATE(SkyDriveService)
    SkyDriveServicePrivate *d_ptr;
};

#endif // SKYDRIVESERVICE_H
