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

#include "skydrivefilelistmodel.h"

#include <QDebug>

class SkyDriveFileListModelPrivate
{
public:
    SkyDriveFileListModelPrivate()
    {
    }

    ~SkyDriveFileListModelPrivate()
    {
    }

private:
    Q_DECLARE_PUBLIC(SkyDriveFileListModel)
    SkyDriveFileListModel *q_ptr;

    QList<QVariantMap> fileList;
};

SkyDriveFileListModel::SkyDriveFileListModel(QObject *parent) :
    QAbstractListModel(parent), d_ptr(new SkyDriveFileListModelPrivate)
{
    d_ptr->q_ptr = this;
}

SkyDriveFileListModel::~SkyDriveFileListModel()
{
    delete d_ptr;
}

int SkyDriveFileListModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const SkyDriveFileListModel);
    if (parent.isValid())
        return 0;
    return d->fileList.count();
}

QVariant SkyDriveFileListModel::data(const QModelIndex &index, int role) const
{
    Q_D(const SkyDriveFileListModel);
    if (index.row() < d->fileList.count()) {
        if (role == Qt::DisplayRole)
            return d->fileList.at(index.row()).value("name", "undefined");
        else if (role == SkyDriveFileListModel::TypeRole)
            return d->fileList.at(index.row()).value("type", "unknown");
        else if (role == SkyDriveFileListModel::IdRole)
            return d->fileList.at(index.row()).value("id", "");
        else if (role == SkyDriveFileListModel::DataRole)
            return d->fileList.at(index.row());
        else if (role == SkyDriveFileListModel::ParentIdRole)
            return d->fileList.at(index.row()).value("parent_id", "");
        else if (role == SkyDriveFileListModel::SourceRole)
            return d->fileList.at(index.row()).value("source", "");
    }
    return QVariant();
}

void SkyDriveFileListModel::setFileListData(const QVariant &data)
{
    Q_D(SkyDriveFileListModel);
    beginResetModel();
    d->fileList.clear();
    QVariantMap dataMap = data.toMap();
    if (dataMap.contains("data")) {
        QVariantList dataList = dataMap.value("data").toList();
        foreach (const QVariant &dataItem, dataList)
            d->fileList.append(dataItem.toMap());
    } else
        d->fileList.append(dataMap);
    endResetModel();
}
