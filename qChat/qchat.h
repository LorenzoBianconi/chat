#ifndef QCHAT_H
#define QCHAT_H

#include <QWidget>
#include <QTcpSocket>
#include <QTextTableFormat>
#include <QListWidgetItem>
#include "msg.h"

namespace Ui {
class qChat;
}

#define MAX_TO_MS   60000
#define DELAY       5000

class qChat : public QWidget
{
    Q_OBJECT
    
public:
    explicit qChat(QWidget *parent = 0);
    ~qChat();
private slots:
    int sndMsg();
    int getMsg();
    int clientAuth();
    int displayError(QAbstractSocket::SocketError err);
    void try_connect();
private:
    Ui::qChat *ui;

    QTimer *_connecTimer;
    QTcpSocket *_sock;
    QString _host, _nick;
    int _port;
    int _attempt;

    QTextTableFormat tableFormat;

    typedef enum {CLIENT_AUTHENTICATED, CLIENT_NOT_AUTHENTICATED} client_ws;
    client_ws _ws;

    QString getHostname();
    int getUserSummary(char *);
    int newUser(QString);
    int userLeft(QListWidgetItem *);
    int displayMsg(QString, QString);

    int mkChatHeader(char *, chat_msg, int);
    int mkSenderHeader(char *, QString);
    int mkChatData(char *, QString);
    int mkAuthReq(char *);
};

#endif // QCHAT_H
