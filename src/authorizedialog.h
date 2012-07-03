#ifndef AUTHORIZEDIALOG_H
#define AUTHORIZEDIALOG_H

#include <QDialog>
#include <QUrl>

class QNetworkReply;

class AuthorizeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AuthorizeDialog(QWidget *parent = 0);
    virtual ~AuthorizeDialog();

protected:
    void requestAuthorizationCode();

private slots:
    void verifyLiveConnectReply(const QUrl &url);
    void obtainRefreshToken(const QString &authorizationCode);
    void checkRefreshTokenResult();

private:
    QNetworkReply *refreshTokenReply;
};

#endif // AUTHORIZEDIALOG_H
