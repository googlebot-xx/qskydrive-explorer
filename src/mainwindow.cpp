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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "settings.h"
#include "liveservices.h"
#include "skydrivefilelistmodel.h"
#include "skydriveservice.h"
#include "authorizedialog.h"

#include <QTimer>
#include <QStack>
#include <QNetworkProxy>
#include <QDesktopServices>
#include <QProgressBar>
#include <QSpacerItem>

class MainWindowPrivate
{
public:
    MainWindowPrivate() :
        q_ptr(0),
        fileListModel(new SkyDriveFileListModel)
    {
    }

    ~MainWindowPrivate()
    {
        delete liveServices;
        Settings::instance()->destroy();
        delete fileListModel;
    }

    void finalizeUI()
    {
        Q_Q(MainWindow);
        q->ui->listView->setModel(fileListModel);
        q->ui->toolBar->setEnabled(false);

        q->connect(q->ui->listView, SIGNAL(doubleClicked(QModelIndex)), q, SLOT(_q_openRemoteItem(QModelIndex)));
        q->connect(q->ui->actionBack, SIGNAL(triggered()), q, SLOT(_q_navigateBack()));
    }

    void applyApplicationProxy()
    {
        QList<QNetworkProxy> systemProxies = QNetworkProxyFactory::systemProxyForQuery();
        foreach (QNetworkProxy proxy, systemProxies) {
            if (proxy.type() == QNetworkProxy::HttpProxy) {
                QNetworkProxy::setApplicationProxy(proxy);
                break;
            }
        }
    }

    void _q_signIn()
    {
        Q_Q(MainWindow);
        QString refreshToken = Settings::instance()->value("live/refreshToken").toString();
        if (refreshToken.isEmpty()) {
            AuthorizeDialog *dialog = new AuthorizeDialog(q);
            if (dialog->exec() == QDialog::Accepted)
                refreshToken = Settings::instance()->value("live/refreshToken").toString();
            else
                q->close();
            delete dialog;
        }
        liveServices = new LiveServices(q);
        q->connect(liveServices, SIGNAL(signInSucceded()), liveServices->skyDriveService(), SLOT(loadFolderList()));
        q->connect(liveServices->skyDriveService(), SIGNAL(folderListLoaded(QVariant)), q, SLOT(_q_displayFolderList(QVariant)));

        liveServices->signIn();
        q->setCursor(QCursor(Qt::BusyCursor));
    }

    void _q_displayFolderList(const QVariant &data)
    {
        Q_Q(MainWindow);
        q->ui->toolBar->setEnabled(true);
        fileListModel->setFileListData(data);
        q->setCursor(QCursor(Qt::ArrowCursor));
        if (folderHierarchyQueue.count() == 0)
            q->ui->actionBack->setEnabled(false);
        else
            q->ui->actionBack->setEnabled(true);
    }

    void _q_openRemoteItem(const QModelIndex &index)
    {
        Q_Q(MainWindow);
        q->setCursor(QCursor(Qt::BusyCursor));
        QString type = index.data(SkyDriveFileListModel::TypeRole).toString();
        if (type == "folder" || type == "album") {
            folderHierarchyQueue.push(index.data(SkyDriveFileListModel::ParentIdRole).toString());
            qDebug() << Q_FUNC_INFO << index.data(SkyDriveFileListModel::DataRole);
            liveServices->skyDriveService()->loadFolderList(index.data(SkyDriveFileListModel::IdRole).toString());
        } else {
            QDesktopServices::openUrl(index.data(SkyDriveFileListModel::SourceRole).toUrl());
            qDebug() << Q_FUNC_INFO << index.data(SkyDriveFileListModel::DataRole);
            q->setCursor(QCursor(Qt::ArrowCursor));
        }
    }

    void _q_navigateBack()
    {
        Q_Q(MainWindow);
        q->setCursor(QCursor(Qt::BusyCursor));
        if (folderHierarchyQueue.count() > 0)
            liveServices->skyDriveService()->loadFolderList(folderHierarchyQueue.pop());
        else
            liveServices->skyDriveService()->loadFolderList();
    }

private:
    Q_DECLARE_PUBLIC(MainWindow)
    MainWindow *q_ptr;

    LiveServices *liveServices;
    SkyDriveFileListModel *fileListModel;

    QStack<QString> folderHierarchyQueue;
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    d_ptr(new MainWindowPrivate),
    ui(new Ui::MainWindow)
{
    d_ptr->q_ptr = this;
    ui->setupUi(this);
    restoreGeometry(Settings::instance()->value("ui/geometry").toByteArray());
    restoreState(Settings::instance()->value("ui/state").toByteArray());

    d_ptr->applyApplicationProxy();
    d_ptr->finalizeUI();

    setCursor(QCursor(Qt::BusyCursor));
    QTimer::singleShot(1000, this, SLOT(_q_signIn()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Settings::instance()->setValue("ui/state", saveState());
    Settings::instance()->setValue("ui/geometry", saveGeometry());
    QMainWindow::closeEvent(event);
}

#include "moc_mainwindow.cpp"
