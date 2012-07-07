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

#include "settings.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

class SettingsPrivate
{
public:
    SettingsPrivate() :
        q_ptr(0)
    {
        settings = new QSettings("ionascu", "qskydrive-explorer");
        settings->setDefaultFormat(QSettings::NativeFormat);
    }

    ~SettingsPrivate()
    {
    }

private:
    Q_DECLARE_PUBLIC(Settings)
    Settings *q_ptr;

    QSettings *settings;
    static Settings *instance;
};
Settings *SettingsPrivate::instance = 0;

Settings::Settings() :
    QObject(), d_ptr(new SettingsPrivate)
{
    d_ptr->q_ptr = this;
}

Settings::~Settings()
{
    d_ptr->settings->sync();
    delete d_ptr;
}

Settings *Settings::instance()
{
    if (!SettingsPrivate::instance)
        SettingsPrivate::instance = new Settings();
    return SettingsPrivate::instance;
}

void Settings::destroy()
{
    SettingsPrivate::instance = 0;
    delete this;
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    Q_D(Settings);
    d->settings->setValue(key, value);
}

QVariant Settings::value(const QString &key)
{
    Q_D(Settings);
    return d->settings->value(key);
}
