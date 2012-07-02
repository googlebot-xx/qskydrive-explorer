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
#include <QDebug>
#include <qjson/parser.h>

#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QNetworkReply>

#include "restclient.h"
#include "settings.h"
#include "skydrivefilelistmodel.h"

class PersistentCookieJar : public QNetworkCookieJar
{
public:
    PersistentCookieJar(QObject *parent = 0) :
        QNetworkCookieJar(parent)
    {
        load();
    }

    virtual ~PersistentCookieJar()
    {
        save();
    }

    void load()
    {
        QByteArray rawCookies = Settings::instance()->cookies();
        if (!rawCookies.isEmpty())
            setAllCookies(QNetworkCookie::parseCookies(rawCookies));
    }

    void save()
    {
        QList<QNetworkCookie> cookies = allCookies();
        QByteArray rawCookies;
        foreach (const QNetworkCookie &cookie, cookies) {
            rawCookies.append(cookie.toRawForm());
            rawCookies.append("\n");
        }
        Settings::instance()->setCookies(rawCookies);
    }
};

class MainWindowPrivate
{
public:
    MainWindowPrivate() :
        q_ptr(0),
        windowContentFrame(0),
        windowContentListView(0),
        signInPopupFrame(0),
        signInPopupWebView(0),
        client(new RestClient),
        pendingNetworkReply(0),
        fileListModel(new SkyDriveFileListModel)
    {
    }

    ~MainWindowPrivate()
    {
        Settings::instance()->destroy();
        delete client;
        delete fileListModel;
    }

    void applyToolBar()
    {
    }

    void applyProxySettings()
    {
        QList<QNetworkProxy> systemProxies = QNetworkProxyFactory::systemProxyForQuery();
        foreach (QNetworkProxy proxy, systemProxies) {
            if (proxy.type() == QNetworkProxy::HttpProxy) {
                QNetworkProxy::setApplicationProxy(proxy);
                break;
            }
        }
    }

    QFrame *windowContent()
    {
        Q_Q(MainWindow);
        if (!windowContentFrame) {
            windowContentFrame = new QFrame(q);
            windowContentListView = new QListView(windowContentFrame);
            QVBoxLayout *layout = new QVBoxLayout(windowContentFrame);
            windowContentFrame->setLayout(layout);
            windowContentFrame->layout()->addWidget(windowContentListView);

            q->connect(windowContentListView, SIGNAL(doubleClicked(QModelIndex)), q, SLOT(openRemoteItem(QModelIndex)));
        }
        return windowContentFrame;
    }

    QFrame *signInPopup()
    {
        Q_Q(MainWindow);
        if (!signInPopupFrame) {
            signInPopupFrame = new QFrame(q, Qt::Dialog);
            signInPopupFrame->setWindowModality(Qt::ApplicationModal);
            if (!signInPopupWebView)
                signInPopupWebView = new QWebView(signInPopupFrame);
            QVBoxLayout *layout = new QVBoxLayout(signInPopupFrame);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(signInPopupWebView);
            signInPopupFrame->setLayout(layout);

            PersistentCookieJar *cookieJar = new PersistentCookieJar(q);
            signInPopupWebView->page()->networkAccessManager()->setCookieJar(cookieJar);
            cookieJar->setParent(q);

            QUrl url;
            url.setHost("login.live.com");
            url.setScheme("https");
            url.setPath("/oauth20_authorize.srf");
            url.addQueryItem("client_id", "000000004C0C2510");
            url.addQueryItem("scope", "wl.signin wl.basic wl.skydrive wl.skydrive_update");
            url.addQueryItem("response_type", "token");
            url.addQueryItem("redirect_url", "https://login.live.com/oauth20_desktop.srf");

            q->connect(signInPopupWebView, SIGNAL(urlChanged(QUrl)), q, SLOT(checkSignInSuccess(QUrl)));
            q->connect(signInPopupWebView, SIGNAL(loadFinished(bool)), q, SLOT(checkRequestsPermissions(bool)));
            signInPopupWebView->load(url);
        }

        return signInPopupFrame;
    }

private:
    Q_DECLARE_PUBLIC(MainWindow)
    MainWindow *q_ptr;

    QFrame *windowContentFrame;
    QListView *windowContentListView;
    QFrame *signInPopupFrame;
    QWebView *signInPopupWebView;

    RestClient *client;
    QNetworkReply *pendingNetworkReply;

    SkyDriveFileListModel *fileListModel;

    QString accessToken;
    QJson::Parser jsonParser;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), d_ptr(new MainWindowPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->applyProxySettings();
    d_ptr->applyToolBar();

    setWindowTitle("QSkyDrive Explorer");

    if (Settings::instance()->value("ui/maximized").toBool())
        setWindowState(Qt::WindowMaximized);
    else
        resize(qMax(Settings::instance()->value("ui/width").toInt(), 300),
               qMax(Settings::instance()->value("ui/height").toInt(), 200));

    setCentralWidget(d_ptr->windowContent());

    signIn();
}

MainWindow::~MainWindow()
{
    Settings::instance()->setValue("ui/width", width());
    Settings::instance()->setValue("ui/height", height());
    Settings::instance()->setValue("ui/maximized", isMaximized());
    delete d_ptr;
}

void MainWindow::signIn()
{
    Q_D(MainWindow);
    if (d->accessToken.isEmpty())
        d->signInPopup();
}

void MainWindow::checkSignInSuccess(const QUrl &url)
{
    Q_D(MainWindow);
    QStringList parameters = url.fragment().split("&");
    QHash<QString, QString> parameterMap;
    foreach (QString parameter, parameters) {
        QStringList parameterPair = parameter.split("=");
        if (parameterPair.count() == 2)
            parameterMap.insert(parameterPair[0], parameterPair[1]);
    }

    if (parameterMap.contains("access_token")) {
        d->accessToken = parameterMap.value("access_token");
        d->signInPopup()->hide();
        qDebug() << Q_FUNC_INFO << "Success!";
        loadFolderList();
    }
}

void MainWindow::checkRequestsPermissions(bool ok)
{
    Q_D(MainWindow);
    Q_UNUSED(ok);
    if (d->accessToken.isEmpty())
        d->signInPopup()->show();
}

void MainWindow::loadFolderList(const QString &folderId)
{
    Q_D(MainWindow);
    QUrl url;
    url.setHost("apis.live.net");
    url.setScheme("https");
    if (folderId.isEmpty())
        url.setPath("/v5.0/me/skydrive");
    else
        url.setPath(QString("/v5.0/%1/files").arg(folderId));
    url.addQueryItem("access_token", d->accessToken);
    d->pendingNetworkReply = d->client->request(url);
    setCursor(QCursor(Qt::BusyCursor));
    connect(d->pendingNetworkReply, SIGNAL(readyRead()), this, SLOT(folderListReady()));
}

void MainWindow::folderListReady()
{
    Q_D(MainWindow);
    d->fileListModel->setFileListData(d->jsonParser.parse(d->pendingNetworkReply));
    d->windowContentListView->setModel(d->fileListModel);
    d->pendingNetworkReply->deleteLater();
    d->pendingNetworkReply = 0;
    setCursor(QCursor(Qt::ArrowCursor));
}

void MainWindow::openRemoteItem(const QModelIndex &index)
{
    QString type = index.data(SkyDriveFileListModel::TypeRole).toString();
    if (type == "folder")
        loadFolderList(index.data(SkyDriveFileListModel::IdRole).toString());
}
