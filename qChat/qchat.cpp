#include "qchat.h"
#include "ui_qchat.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>

qChat::qChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::qChat)
{
    ui->setupUi(this);

    _port = 9999;
    _sock = new QTcpSocket(this);
    _ws = CLIENT_NOT_AUTHENTICATED;

    ui->sndButton->setEnabled(false);
    ui->disconnectButton->setEnabled(false);

    connect(ui->msgEdit, SIGNAL(returnPressed()), this, SLOT(snd_txt_msg()));
    connect(ui->sndButton, SIGNAL(clicked()), this, SLOT(snd_txt_msg()));
    connect(_sock, SIGNAL(readyRead()), this, SLOT(get_msg()));
    connect(ui->disconnectButton, SIGNAL(clicked()), this, SLOT(close_sock()));
    connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(conn2server()));
    connect(_sock, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(display_error(QAbstractSocket::SocketError)));
}

qChat::~qChat()
{
    delete ui;
}

int qChat::mk_chat_header(char *msg, enum chat_msg type, QString nick)
{
    struct chat_header *ch = (struct chat_header *) msg;
    ch->type = type;
    memcpy(ch->nick, nick.toLatin1().data(), nick.size());
    return 0;
}

int qChat::mk_chat_data(char *msg, QString data)
{
    struct chat_data *d = (struct chat_data *)(msg + sizeof(struct chat_header));
    memcpy(d->data, data.toLatin1().data(), data.size());
    return 0;
}

int qChat::mk_auth_req(char *msg)
{
    return 0;
}


int qChat::snd_msg(char *msg)
{
    msg[BUFFLEN - 1] = 0x0a;
    _sock->write(msg, BUFFLEN);
    return 0;
}

int qChat::display_msg(QString nick, QString data, enum chat_msg msg_type)
{
    QTime time = QTime::currentTime();
    QString msg = "[" + time.toString() + "] ";
    switch (msg_type) {
    case CHAT_DATA:
        msg += (nick + ": "+ data);
        break;
    case CHAT_USET_CONN:
        msg += ("=========> " + nick + " is connected");
        break;
    case CHAT_USER_DISC:
        msg += ("<========= " + nick + " is disconnected");
        break;
    default:
        break;
    }
    msg += "\n";
    QTextCursor cursor(ui->textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(msg);
    QScrollBar *bar = ui->textEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
    return 0;
}

int qChat::get_msg()
{
    char buff[BUFFLEN];
    memset(buff, 0, BUFFLEN);

    _sock->read(buff, BUFFLEN);

    struct chat_header *ch = (struct chat_header *) buff;
    switch (ch->type) {
    case CHAT_AUTH_REP: {
        struct chat_auth_rep *res = (struct chat_auth_rep *)(ch + 1);
        if (res->res_type == AUTH_SUCCESS) {
            _ws = CLIENT_AUTHENTICATED;
            ui->connectButton->setEnabled(false);
            ui->disconnectButton->setEnabled(true);
            ui->sndButton->setEnabled(true);
        } else
            QMessageBox::information(this, "Error", "Authentication failed");
        break;
    }
    case CHAT_DATA: {
        struct chat_data *data = (struct chat_data *)(ch + 1);
        display_msg(QString(ch->nick), QString(data->data), CHAT_DATA);
        break;
    }
    case CHAT_USET_CONN:
        display_msg(QString(ch->nick) , "", CHAT_USET_CONN);
        break;
    case CHAT_USER_DISC:
        display_msg(QString(ch->nick) , "", CHAT_USER_DISC);
        break;
    default:
        break;
    }
    return 0;
}

int qChat::conn2server()
{
    if (ui->nickEdit->text().isEmpty() && ui->serverEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Warning", "Missing server and nick info");
            return -1;
    } else if (ui->nickEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Warning", "Missing nick info");
            return -1;
    }
    if (ui->serverEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Warning", "Missing server info");
            return -1;
    }

    _host = ui->serverEdit->text();
    _nick = ui->nickEdit->text();
    if (!ui->portEdit->text().isEmpty())
        _port = ui->portEdit->text().toInt();
    _sock->connectToHost(_host, _port);
    client_auth();
    return 0;
}

int qChat::client_auth()
{
    char buff[BUFFLEN];
    memset(buff, 0, BUFFLEN);
    /*
     * XXX: open authentication for the moment
     */
    mk_chat_header(buff, CHAT_AUTH_REQ, _nick);
    mk_auth_req(buff);
    snd_msg(buff);
    return 0;
}

int qChat::snd_txt_msg()
{
    if (_ws == CLIENT_AUTHENTICATED) {
        char buff[BUFFLEN];
        memset(buff, 0, BUFFLEN);
        QString data = ui->msgEdit->text();
        display_msg(_nick, data, CHAT_DATA);
        mk_chat_header(buff, CHAT_DATA, _nick);
        mk_chat_data(buff, data);
        snd_msg(buff);
        ui->msgEdit->clear();
        return 0;
    }
    return -1;
}

int qChat::display_error(QAbstractSocket::SocketError err)
{
    switch (err) {
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, "Error", "server not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, "Error", "connection refused");
        break;
    default:
        break;
    }
    return 0;
}

int qChat::close_sock()
{
    _ws = CLIENT_NOT_AUTHENTICATED;
    _sock->close();
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
    ui->sndButton->setEnabled(false);
    return 0;
}
