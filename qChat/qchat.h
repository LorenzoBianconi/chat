#ifndef QCHAT_H
#define QCHAT_H

#include <QWidget>
#include <QTcpSocket>
#include "msg.h"

namespace Ui {
class qChat;
}

class qChat : public QWidget
{
    Q_OBJECT
    
public:
    explicit qChat(QWidget *parent = 0);
    ~qChat();
private slots:
    int snd_txt_msg();
    int get_msg();
    int conn2server();
    int display_error(QAbstractSocket::SocketError err);
    int close_sock();
private:
    Ui::qChat *ui;

    QTcpSocket *_sock;
    QByteArray _buffer;

    QString _host, _nick;
    int _port;

    enum client_ws {CLIENT_AUTHENTICATED, CLIENT_NOT_AUTHENTICATED};
    enum client_ws _ws;

    int snd_msg(char *);
    int display_msg(QString, QString, enum chat_msg);
    int client_auth();
    int mk_chat_header(char *, enum chat_msg, QString);
    int mk_chat_data(char *, QString);
    int mk_auth_req(char *);
};

#endif // QCHAT_H
