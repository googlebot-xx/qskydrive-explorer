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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QVariantList>

class SettingsPrivate;

class Settings : public QObject
{
    Q_OBJECT
public:
    virtual ~Settings();
    static Settings *instance();

    void destroy();

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key);

private:
    Settings();

private:
    Q_DECLARE_PRIVATE(Settings)
    SettingsPrivate *d_ptr;
};

#endif // SETTINGS_H
