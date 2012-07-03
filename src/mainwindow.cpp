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

#include <QFrame>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListView>
#include <QWebView>
#include <QToolBar>
#include <QToolButton>
#include <QIcon>
#include <QAction>
#include <QMessageBox>
#include <qjson/parser.h>
#include <QDesktopServices>

#include <QStack>

#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QNetworkReply>

#include "restclient.h"
#include "settings.h"
#include "skydrivefilelistmodel.h"
#include "liveservices.h"
#include "skydriveservice.h"
#include "authorizedialog.h"

#include <QDebug>

class MainWindowPrivate
{
public:
    MainWindowPrivate() :
        q_ptr(0),
        backAction(0),
        windowContentListView(0),
        pendingNetworkReply(0),
        fileListModel(new SkyDriveFileListModel)
    {
    }

    ~MainWindowPrivate()
    {
        delete liveServices;
        Settings::instance()->destroy();
        delete fileListModel;
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

    void createToolBar()
    {
        Q_Q(MainWindow);
        if (!toolBar) {
            toolBar = q->addToolBar("Navigation");
            backAction = toolBar->addAction(QIcon::fromTheme("back"), "Back", q, SLOT(navigateBack()));
            backAction->setEnabled(false);
            q->addToolBar(toolBar);
        }
    }

    QListView *windowContent()
    {
        Q_Q(MainWindow);
        if (!windowContentListView) {
            windowContentListView = new QListView(q);
            windowContentListView->setModel(fileListModel);
            q->connect(windowContentListView, SIGNAL(doubleClicked(QModelIndex)), q, SLOT(openRemoteItem(QModelIndex)));
        }
        return windowContentListView;
    }

private:
    Q_DECLARE_PUBLIC(MainWindow)
    MainWindow *q_ptr;

    LiveServices *liveServices;

    QAction *backAction;

    QToolBar *toolBar;
    QListView *windowContentListView;

    RestClient *client;
    QNetworkReply *pendingNetworkReply;

    SkyDriveFileListModel *fileListModel;

    QStack<QString> folderHierarchyQueue;
    QJson::Parser jsonParser;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), d_ptr(new MainWindowPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->applyApplicationProxy();
    d_ptr->createToolBar();

    setWindowTitle("QSkyDrive Explorer");

    setContentsMargins(6, 6, 6, 6);
    setCentralWidget(d_ptr->windowContent());

    restoreGeometry(Settings::instance()->value("ui/geometry").toByteArray());
    restoreState(Settings::instance()->value("ui/state").toByteArray());

    signIn();
}

MainWindow::~MainWindow()
{

    delete d_ptr;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Settings::instance()->setValue("ui/state", saveState());
    Settings::instance()->setValue("ui/geometry", saveGeometry());
    QMainWindow::closeEvent(event);
}

void MainWindow::signIn()
{
    Q_D(MainWindow);
    QString refreshToken = Settings::instance()->value("live/refreshToken").toString();
    if (refreshToken.isEmpty()) {
        AuthorizeDialog *dialog = new AuthorizeDialog(this);
        if (dialog->exec() == QDialog::Accepted)
            refreshToken = Settings::instance()->value("live/refreshToken").toString();
        else
            close();
        delete dialog;
    }
    d->liveServices = new LiveServices(this);
    connect(d->liveServices, SIGNAL(signInSucceded()), d->liveServices->skyDriveService(), SLOT(loadFolderList()));
    connect(d->liveServices->skyDriveService(), SIGNAL(folderListLoaded(QVariant)), this, SLOT(displayFolderList(QVariant)));

    d->liveServices->signIn();
    setCursor(QCursor(Qt::BusyCursor));
}

void MainWindow::displayFolderList(const QVariant &data)
{
    Q_D(MainWindow);
    d->fileListModel->setFileListData(data);
    setCursor(QCursor(Qt::ArrowCursor));
    if (d->folderHierarchyQueue.count() == 0)
        d->backAction->setEnabled(false);
    else
        d->backAction->setEnabled(true);
}

void MainWindow::openRemoteItem(const QModelIndex &index)
{
    Q_D(MainWindow);
    setCursor(QCursor(Qt::BusyCursor));
    QString type = index.data(SkyDriveFileListModel::TypeRole).toString();
    if (type == "folder") {
        d->folderHierarchyQueue.push(index.data(SkyDriveFileListModel::ParentIdRole).toString());
        qDebug() << Q_FUNC_INFO << index.data(SkyDriveFileListModel::DataRole);
        d->liveServices->skyDriveService()->loadFolderList(index.data(SkyDriveFileListModel::IdRole).toString());
    } else {
        QDesktopServices::openUrl(index.data(SkyDriveFileListModel::SourceRole).toUrl());
        qDebug() << Q_FUNC_INFO << index.data(SkyDriveFileListModel::DataRole);
        setCursor(QCursor(Qt::ArrowCursor));
    }
}

void MainWindow::navigateBack()
{
    Q_D(MainWindow);
    qDebug() << Q_FUNC_INFO;
    setCursor(QCursor(Qt::BusyCursor));
    if (d->folderHierarchyQueue.count() > 0)
        d->liveServices->skyDriveService()->loadFolderList(d->folderHierarchyQueue.pop());
    else
        d->liveServices->skyDriveService()->loadFolderList();
}
