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
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>

class MainWindowPrivate
{
public:
    MainWindowPrivate() :
        q_ptr(0),
        fileListModel(new SkyDriveFileListModel),
        newFolderDialog(0),
        quotaProgressBar(0),
        uploadProgressDialog(0)
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

        quotaProgressBar = new QProgressBar(q->ui->toolBar);
        quotaProgressBar->setMinimum(0);
        quotaProgressBar->setMaximum(0);

        uploadProgressDialog = new QProgressDialog(q);
        uploadProgressDialog->setWindowTitle("Uploading file(s)");
        uploadProgressDialog->setMinimum(0);
        uploadProgressDialog->setMaximum(100);
        uploadProgressDialog->setAutoClose(false);
        uploadProgressDialog->setAutoReset(false);

        q->ui->toolBar->addWidget(createSpacer(q->ui->toolBar));
        q->ui->toolBar->addWidget(quotaProgressBar);

        q->ui->listView->setModel(fileListModel);
        q->ui->toolBar->setEnabled(false);

        q->ui->actionRemove->setEnabled(false);
        q->ui->actionForward->setEnabled(false);

        q->connect(q->ui->listView, SIGNAL(doubleClicked(QModelIndex)), q, SLOT(_q_openRemoteItem(QModelIndex)));
        q->connect(q->ui->listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), q, SLOT(_q_updateFileSelection(QItemSelection,QItemSelection)));
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

    QWidget *createSpacer(QWidget *parent = 0)
    {
        QWidget *spacer = new QWidget(parent);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        return spacer;
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
        liveServices = new LiveServices(refreshToken, q);
        q->connect(liveServices, SIGNAL(signInSucceded()), liveServices->skyDriveService(), SLOT(loadFolderList()));
        q->connect(liveServices, SIGNAL(signInSucceded()), liveServices->skyDriveService(), SLOT(updateUserQuota()));
        q->connect(liveServices->skyDriveService(), SIGNAL(folderListLoaded(QVariant)), q, SLOT(_q_displayFolderList(QVariant)));
        q->connect(liveServices->skyDriveService(), SIGNAL(folderListUpdated()), q, SLOT(_q_refreshFolderList()));
        q->connect(liveServices->skyDriveService(), SIGNAL(userQuotaUpdated(QVariant)), q, SLOT(_q_displayUserQuota(QVariant)));
        q->connect(liveServices->skyDriveService(), SIGNAL(itemRemoved()), q, SLOT(_q_refreshFolderList()));
        q->connect(liveServices->skyDriveService(), SIGNAL(itemUploaded()), q, SLOT(_q_refreshFolderList()));
        q->connect(liveServices->skyDriveService(), SIGNAL(itemUploadProgress(qint64,qint64)), q, SLOT(_q_updateUploadProgress(qint64,qint64)));
        q->connect(liveServices->skyDriveService(), SIGNAL(itemUploaded()), q, SLOT(_q_closeUploadProgressDialog()));

        q->connect(uploadProgressDialog, SIGNAL(rejected()), liveServices->skyDriveService(), SLOT(cancelItemUpload()));

        liveServices->signIn();
        q->setCursor(QCursor(Qt::BusyCursor));
    }

    void _q_displayFolderList(const QVariant &data)
    {
        Q_Q(MainWindow);
        q->ui->toolBar->setEnabled(true);
        fileListModel->setFileListData(data);
        q->setCursor(QCursor(Qt::ArrowCursor));
        if (navigationStack.isEmpty())
            q->ui->actionBack->setEnabled(false);
        else
            q->ui->actionBack->setEnabled(true);

        if (reverseNavigationStack.isEmpty())
            q->ui->actionForward->setEnabled(false);
        else
            q->ui->actionForward->setEnabled(true);

        q->ui->actionRemove->setEnabled(false);
    }

    void _q_openRemoteItem(const QModelIndex &index)
    {
        Q_Q(MainWindow);
        q->setCursor(QCursor(Qt::BusyCursor));
        QString type = index.data(SkyDriveFileListModel::TypeRole).toString();
        if (type == "folder" || type == "album") {
            currentFolderId = index.data(SkyDriveFileListModel::IdRole).toString();
            navigationStack.push(index.data(SkyDriveFileListModel::ParentIdRole).toString());
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
        if (navigationStack.isEmpty()) {
            liveServices->skyDriveService()->loadFolderList();
        } else {
            reverseNavigationStack.push(currentFolderId);
            currentFolderId = navigationStack.pop();
            liveServices->skyDriveService()->loadFolderList(currentFolderId);
        }
        q->ui->actionRemove->setEnabled(false);
    }

    void _q_navigateForward()
    {
        Q_Q(MainWindow);
        q->setCursor(QCursor(Qt::BusyCursor));
        if (reverseNavigationStack.isEmpty()) {
            liveServices->skyDriveService()->loadFolderList();
        } else {
            navigationStack.push(currentFolderId);
            currentFolderId = reverseNavigationStack.pop();
            liveServices->skyDriveService()->loadFolderList(currentFolderId);
        }
        q->ui->actionRemove->setEnabled(false);
    }

    void _q_navigateHome()
    {
        Q_Q(MainWindow);
        q->setCursor(QCursor(Qt::BusyCursor));
        navigationStack.clear();
        reverseNavigationStack.clear();
        liveServices->skyDriveService()->loadFolderList();
        q->ui->actionRemove->setEnabled(false);
    }

    void _q_createFolder()
    {
        if (newFolderDialog->exec() == QDialog::Accepted)
            liveServices->skyDriveService()->createFolder(currentFolderId, newFolderDialog->textValue());
    }

    void _q_uploadFiles()
    {
        Q_Q(MainWindow);
        QString fileName = QFileDialog::getOpenFileName(q, "Upload file(s)");
        qDebug() << "Files to upload:" << fileName;
        liveServices->skyDriveService()->uploadItem(currentFolderId, fileName);
        uploadProgressDialog->open();
    }

    void _q_removeFiles()
    {
        Q_Q(MainWindow);
        QMessageBox messsageBox(QMessageBox::Question, "Are you sure?", "Do you really want to remove this file or folder?",
                                QMessageBox::Yes | QMessageBox::No, q);
        if (messsageBox.exec() == QMessageBox::Yes) {
            liveServices->skyDriveService()->removeItem(q->ui->listView->selectionModel()->selectedIndexes().at(0).data(SkyDriveFileListModel::IdRole).toString());
        }
        qDebug() << messsageBox.result();
    }

    void _q_refreshFolderList()
    {
        liveServices->skyDriveService()->loadFolderList(currentFolderId);
    }

    void _q_updateFileSelection(const QItemSelection &selected, QItemSelection &deselected)
    {
        Q_Q(MainWindow);
        Q_UNUSED(deselected);
        if (selected.indexes().count() == 0)
            q->ui->actionRemove->setEnabled(false);
        else
            q->ui->actionRemove->setEnabled(true);
    }

    void _q_displayUserQuota(const QVariant &data)
    {
        QVariantMap quota = data.toMap();
        quotaProgressBar->setMaximum(100);
        quotaProgressBar->setMinimum(0);
        quotaProgressBar->setValue(100 - quota["available"].toDouble() / quota["quota"].toDouble() * 100.0);
    }

    void _q_updateUploadProgress(qint64 bytesSent, qint64 bytesTotal)
    {
        qDebug() << Q_FUNC_INFO << bytesSent << bytesTotal;
        uploadProgressDialog->setValue((double)bytesSent/(double)bytesTotal * 100);
    }

    void _q_closeUploadProgressDialog()
    {
        qDebug() << Q_FUNC_INFO;
        uploadProgressDialog->close();
    }

private:
    Q_DECLARE_PUBLIC(MainWindow)
    MainWindow *q_ptr;

    LiveServices *liveServices;
    SkyDriveFileListModel *fileListModel;
    QInputDialog *newFolderDialog;
    QProgressBar *quotaProgressBar;
    QProgressDialog *uploadProgressDialog;

    QStack<QString> navigationStack;
    QStack<QString> reverseNavigationStack;
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
