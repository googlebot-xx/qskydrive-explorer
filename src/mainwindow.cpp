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
#include <QInputDialog>

class MainWindowPrivate
{
public:
    MainWindowPrivate() :
        q_ptr(0),
        fileListModel(new SkyDriveFileListModel),
        newFolderDialog(0)
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
        newFolderDialog = new QInputDialog(q);
        newFolderDialog->setWindowTitle("Create new folder");
        newFolderDialog->setLabelText("Folder name:");

        q->ui->listView->setModel(fileListModel);
        q->ui->toolBar->setEnabled(false);

        q->connect(q->ui->listView, SIGNAL(doubleClicked(QModelIndex)), q, SLOT(_q_openRemoteItem(QModelIndex)));
        q->connect(q->ui->actionBack, SIGNAL(triggered()), q, SLOT(_q_navigateBack()));
        q->connect(q->ui->actionForward, SIGNAL(triggered()), q, SLOT(_q_navigateForward()));
        q->connect(q->ui->actionHome, SIGNAL(triggered()), q, SLOT(_q_navigateHome()));
        q->connect(q->ui->actionNewFolder, SIGNAL(triggered()), q, SLOT(_q_createFolder()));
        q->connect(q->ui->actionUpload, SIGNAL(triggered()), q, SLOT(_q_uploadFiles()));
        q->connect(q->ui->actionRemove, SIGNAL(triggered()), q, SLOT(_q_removeFiles()));
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
        q->connect(liveServices->skyDriveService(), SIGNAL(folderListUpdated()), q, SLOT(_q_refreshFolderList()));

        liveServices->signIn();
        q->setCursor(QCursor(Qt::BusyCursor));
    }

    void _q_displayFolderList(const QVariant &data)
    {
        Q_Q(MainWindow);
        q->ui->toolBar->setEnabled(true);
        fileListModel->setFileListData(data);
        q->setCursor(QCursor(Qt::ArrowCursor));
        if (folderHierarchyStack.count() == 0)
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
            currentFolderId = index.data(SkyDriveFileListModel::IdRole).toString();
            folderHierarchyStack.push(index.data(SkyDriveFileListModel::ParentIdRole).toString());
            qDebug() << Q_FUNC_INFO << index.data(SkyDriveFileListModel::DataRole);
            liveServices->skyDriveService()->loadFolderList(currentFolderId);
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
        if (folderHierarchyStack.count() > 0) {
            currentFolderId = folderHierarchyStack.pop();
            liveServices->skyDriveService()->loadFolderList(currentFolderId);
        } else
            liveServices->skyDriveService()->loadFolderList();
    }

    void _q_navigateForward()
    {
        // TODO: Implement forward step navigation
    }

    void _q_navigateHome()
    {
        Q_Q(MainWindow);
        q->setCursor(QCursor(Qt::BusyCursor));
        liveServices->skyDriveService()->loadFolderList();
    }

    void _q_createFolder()
    {
        if (newFolderDialog->exec() == QDialog::Accepted)
            liveServices->skyDriveService()->createFolder(currentFolderId, newFolderDialog->textValue());
    }

    void _q_uploadFiles()
    {
        // TODO: Implement file upload
    }

    void _q_removeFiles()
    {
        // TODO: Implement file removal
    }

    void _q_refreshFolderList()
    {
        liveServices->skyDriveService()->loadFolderList(currentFolderId);
    }

private:
    Q_DECLARE_PUBLIC(MainWindow)
    MainWindow *q_ptr;

    LiveServices *liveServices;
    SkyDriveFileListModel *fileListModel;
    QInputDialog *newFolderDialog;

    QStack<QString> folderHierarchyStack;
    QString currentFolderId;
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
