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

class qChat : public QWidget
{
    Q_OBJECT
    
public:
    explicit qChat(QWidget *parent = 0);
    ~qChat();
private slots:
    int snd_txt_msg();
    int get_msg();
    int client_connected();
    int display_error(QAbstractSocket::SocketError err);
private:
    Ui::qChat *ui;

    QTcpSocket *_sock;
    QString _host, _nick;
    int _port;

    QTextTableFormat tableFormat;

    enum client_ws {CLIENT_AUTHENTICATED, CLIENT_NOT_AUTHENTICATED};
    enum client_ws _ws;

    QString get_hostname();
    int get_users_summary(char *);
    int snd_msg(char *);
    int new_user(QString);
    int user_left(QListWidgetItem *);
    int display_msg(QString, QString);
    int client_auth();
    int mk_chat_header(char *, enum chat_msg, int, QString);
    int mk_chat_data(char *, QString);
    int mk_auth_req(char *);
};

#endif // QCHAT_H
