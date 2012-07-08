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
    void folderListUpdated();
    void userQuotaUpdated(const QVariant &data);

public slots:
    void loadFolderList(const QString &folderId = "");
    void createFolder(const QString &parentId, const QString &folderName);
    void updateUserQuota();

private:
    Q_PRIVATE_SLOT(d_func(), void _q_folderListReady())
    Q_PRIVATE_SLOT(d_func(), void _q_folderCreateReady())
    Q_PRIVATE_SLOT(d_func(), void _q_userQuotaReady())

private:
    Q_DECLARE_PRIVATE(SkyDriveService)
    SkyDriveServicePrivate *d_ptr;
};

#endif // SKYDRIVESERVICE_H
