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

#ifndef SKYDRIVEFILELISTMODEL_H
#define SKYDRIVEFILELISTMODEL_H

#include <QAbstractListModel>

class SkyDriveFileListModelPrivate;

class SkyDriveFileListModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum Roles {
        TypeRole = Qt::UserRole + 1,
        IdRole
    };

    explicit SkyDriveFileListModel(QObject *parent = 0);
    virtual ~SkyDriveFileListModel();

    virtual int rowCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;

    void setFileListData(const QVariant &data);

private:
    Q_DECLARE_PRIVATE(SkyDriveFileListModel)
    SkyDriveFileListModelPrivate *d_ptr;
};

#endif // SKYDRIVEFILELISTMODEL_H
