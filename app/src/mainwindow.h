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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindowPrivate;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
protected:
    void closeEvent(QCloseEvent *event);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_signIn())
    Q_PRIVATE_SLOT(d_func(), void _q_displayFolderList(QVariant))
    Q_PRIVATE_SLOT(d_func(), void _q_openRemoteItem(QModelIndex))
    Q_PRIVATE_SLOT(d_func(), void _q_navigateBack())
    Q_PRIVATE_SLOT(d_func(), void _q_navigateForward())
    Q_PRIVATE_SLOT(d_func(), void _q_navigateHome())
    Q_PRIVATE_SLOT(d_func(), void _q_createFolder())
    Q_PRIVATE_SLOT(d_func(), void _q_uploadFiles())
    Q_PRIVATE_SLOT(d_func(), void _q_removeFiles())
    Q_PRIVATE_SLOT(d_func(), void _q_refreshFolderList())
    Q_PRIVATE_SLOT(d_func(), void _q_updateFileSelection(QItemSelection,QItemSelection))
    Q_PRIVATE_SLOT(d_func(), void _q_displayUserQuota(QVariant))

private:
    Q_DECLARE_PRIVATE(MainWindow)
    MainWindowPrivate *d_ptr;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
