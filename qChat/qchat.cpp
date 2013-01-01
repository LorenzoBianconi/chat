#include "qchat.h"
#include "ui_qchat.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QTextTable>
#include <QHostInfo>
#include <QProcess>
#include <QtEndian>

qChat::qChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::qChat)
{
    ui->setupUi(this);

    _host = "lorenet.dyndns.org";
    _port = 9999;
    _nick = get_hostname() + QString("@") + QHostInfo::localHostName();
    _sock = new QTcpSocket(this);
    _ws = CLIENT_NOT_AUTHENTICATED;

    tableFormat.setBorder(0);

    connect(ui->msgEdit, SIGNAL(returnPressed()), this, SLOT(snd_txt_msg()));
    connect(_sock, SIGNAL(connected()), this, SLOT(client_connected()));
    connect(_sock, SIGNAL(readyRead()), this, SLOT(get_msg()));
    connect(_sock, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(display_error(QAbstractSocket::SocketError)));

    _sock->connectToHost(_host, _port);
}

qChat::~qChat()
{
    delete ui;
}

QString qChat::get_hostname()
{
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";

    QStringList environment = QProcess::systemEnvironment();
    foreach (QString string, envVariables) {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1) {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2) {
                return stringList.at(1).toUtf8();
            }
        }
    }
    return QString();
}

int qChat::mk_chat_header(char *msg, enum chat_msg type, int msglen, QString nick)
{
    struct chat_header *ch = (struct chat_header *) msg;
    ch->type = qToBigEndian((int)type);
    ch->len = qToBigEndian((int)sizeof(struct chat_header) + msglen);
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

int qChat::display_msg(QString nick, QString data)
{
    QTextCursor cursor(ui->textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    QTextTable *table = cursor.insertTable(1, 2, tableFormat);
    table->cellAt(0, 0).firstCursorPosition().insertText('<' + nick + "> ");
    table->cellAt(0, 1).firstCursorPosition().insertText(data);
    QScrollBar *bar = ui->textEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
    return 0;
}

int qChat::new_user(QString nick)
{
    QColor color = ui->textEdit->textColor();
    ui->textEdit->setTextColor(Qt::gray);
    ui->textEdit->append(tr("* %1 has joined").arg(nick));
    ui->textEdit->setTextColor(color);
    ui->userList->addItem(nick);
    return 0;
}

int qChat::user_left(QListWidgetItem *item)
{
    QString nick = item->text();
    delete item;
    QColor color = ui->textEdit->textColor();
    ui->textEdit->setTextColor(Qt::gray);
    ui->textEdit->append(tr("* %1 has left").arg(nick));
    ui->textEdit->setTextColor(color);
    return 0;
}

int qChat::get_users_summary(char *ptr)
{
    struct chat_header *ch = (struct chat_header *) ptr;
    struct chat_user_summary *tmp = (struct chat_user_summary *)(ch + 1);
    struct chat_user_summary *end = (struct chat_user_summary *)(ptr + qFromBigEndian(ch->len));

    QList<QString> list;
    while (tmp < end) {
        list << QString(tmp->nick);
        tmp++;
    }

    foreach (QString n, list) {
        QList<QListWidgetItem *> items = ui->userList->findItems(n, Qt::MatchExactly);
        if (items.isEmpty())
            new_user(n);
    }

    for (int i = 0; i < ui->userList->count(); i++) {
        bool found = false;
        QListWidgetItem *item = ui->userList->item(i);
        QString n = item->text();
        for (int j = 0; j < list.size(); j++) {
            if (list.at(j) == n) {
                found = true;
                break;
            }
        }
        if (found == false && !n.isEmpty())
            user_left(item);
    }
    return 0;
}

int qChat::get_msg()
{
    char buff[BUFFLEN];
    memset(buff, 0, BUFFLEN);

    _sock->read(buff, BUFFLEN);

    struct chat_header *ch = (struct chat_header *) buff;
    switch (qFromBigEndian((int)ch->type)) {
    case CHAT_AUTH_REP: {
        struct chat_auth_rep *res = (struct chat_auth_rep *)(ch + 1);
        if (qFromBigEndian((int)res->res_type) == AUTH_SUCCESS) {
            _ws = CLIENT_AUTHENTICATED;
        } else
            QMessageBox::information(this, "Error", "Authentication failed");
        break;
    }
    case CHAT_DATA: {
        struct chat_data *data = (struct chat_data *)(ch + 1);
        display_msg(QString(ch->nick), QString(data->data));
        break;
    }
    case CHAT_USER_SUMMARY:
        get_users_summary(buff);
        break;
    default:
        break;
    }
    return 0;
}

int qChat::client_connected()
{
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
    mk_chat_header(buff, CHAT_AUTH_REQ, sizeof(struct chat_auth_req), _nick);
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
        display_msg(_nick, data);
        mk_chat_header(buff, CHAT_DATA, data.size(), _nick);
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
