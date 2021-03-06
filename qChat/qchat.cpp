#include "qchat.h"
#include "ui_qchat.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QTextTable>
#include <QHostInfo>
#include <QProcess>
#include <QtEndian>
#include <QTimer>

qChat::qChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::qChat)
{
    ui->setupUi(this);

    /* server parameters */
    _host = "achat.lorenzobianconi.net";
    _port = 9999;
    _nick = getHostname() + QString("@") + QHostInfo::localHostName();
    _sock = new QTcpSocket(this);
    _connecTimer = new QTimer(this);

    _ws = CLIENT_NOT_AUTHENTICATED;

    _attempt = 1;

    tableFormat.setBorder(0);

    connect(ui->msgEdit, SIGNAL(returnPressed()), this, SLOT(sndMsg()));
    connect(_sock, SIGNAL(connected()), this, SLOT(clientAuth()));
    connect(_sock, SIGNAL(readyRead()), this, SLOT(getMsg()));
    connect(_sock, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(_connecTimer, SIGNAL(timeout()), this, SLOT(try_connect()));

    _sock->connectToHost(_host, _port);
    _connecTimer->setSingleShot(true);
}

qChat::~qChat()
{
    delete ui;
}

QString qChat::getHostname()
{
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";

    QStringList environment = QProcess::systemEnvironment();

    foreach (QString string, envVariables) {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1) {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2)
                return stringList.at(1).toUtf8();
        }
    }
    return QString();
}

int qChat::mkChatHeader(char *msg,chat_msg type, int msglen)
{
    chat_header *ch = (chat_header *)msg;

    ch->type = qToBigEndian((int)type);
    ch->dlen = qToBigEndian(msglen);

    return 0;
}

int qChat::mkSenderHeader(char *msg, QByteArray nick)
{
    char *sinfo = (char *)(msg + sizeof(chat_header));

    *((int *)sinfo) = qToBigEndian(nick.size());
    memcpy(sinfo + 4, nick.data(), nick.size());

    return 0;
}

int qChat::mkChatData(char *msg, QByteArray text)
{
    char *sInfo = (char *)(msg + sizeof(chat_header));
    char *data = (char *)(sInfo + 4 + _nick.size());

    memcpy(data, text.data(), text.size());

    return 0;
}

int qChat::mkAuthReq(char *msg)
{
    return 0;
}

int qChat::displayMsg(QString nick, QString data)
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

int qChat::newUser(QString nick)
{
    QColor color = ui->textEdit->textColor();
    ui->textEdit->setTextColor(Qt::gray);
    ui->textEdit->append(tr("* %1 has joined").arg(nick));
    ui->textEdit->setTextColor(color);

    QListWidgetItem *item = new QListWidgetItem(nick);
    if (nick == _nick)
        item->setTextColor(Qt::green);
    ui->userList->addItem(item);

    return 0;
}

int qChat::userLeft(QListWidgetItem *item)
{
    QString nick = item->text();
    delete item;

    QColor color = ui->textEdit->textColor();
    ui->textEdit->setTextColor(Qt::gray);
    ui->textEdit->append(tr("* %1 has left").arg(nick));
    ui->textEdit->setTextColor(color);

    return 0;
}

int qChat::getUserSummary(char *msg)
{
    chat_header *ch = (chat_header *) msg;
    char *tmp, *end, *sInfo = (char *)(ch + 1);

    tmp = sInfo + qFromBigEndian(*((int *)sInfo)) + 4;
    end = sInfo + qFromBigEndian(ch->dlen);

    QList<QString> list;
    while (tmp < end) {
        int nicklen = qFromBigEndian(*((int *) tmp));
        tmp += 4;
        list << QString(tmp);
        tmp += nicklen;
    }

    foreach (QString n, list) {
        QList<QListWidgetItem *> items = ui->userList->findItems(n, Qt::MatchExactly);
        if (items.isEmpty())
            newUser(n);
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
            userLeft(item);
    }

    return 0;
}

int qChat::getMsg()
{
    char buff[BUFFLEN];
    memset(buff, 0, BUFFLEN);

    int len = _sock->read(buff, BUFFLEN);
    if (len < 0) {
        qDebug() << "Error while reading from socket";
        return -1;
    }

    chat_header *ch = (chat_header *)buff;
    int type = qFromBigEndian(ch->type);
    int dlen = qFromBigEndian(ch->dlen);
    char *sInfo = (char *)(ch + 1);
    int nicklen = qFromBigEndian(*((int *)sInfo));
    char *data = (char *)(sInfo + 4 + nicklen);

    switch (type) {
    case CHAT_AUTH_REP: {
        chat_auth_rep *res = (chat_auth_rep *)data;

        if (qFromBigEndian((int) res->res_type) == AUTH_SUCCESS)
            _ws = CLIENT_AUTHENTICATED;
        else {
            QMessageBox::information(this, "Error", "Authentication failed");
            this->close();
        }
        break;
    }
    case CHAT_DATA: {
        int len = dlen - (4 + nicklen);
        displayMsg(QString(sInfo + 4).mid(0, nicklen), QString(data).mid(0, len));
        break;
    }
    case CHAT_USER_SUMMARY: {
        getUserSummary(buff);
        break;
    }
    default:
        break;
    }
    return 0;
}

int qChat::clientAuth()
{
    int datalen = 4 + _nick.size() + sizeof(chat_auth_req);
    int bufflen = sizeof(chat_header) + datalen;
    char buff[bufflen];
    memset(buff, 0, bufflen);
    /*
     * XXX: open authentication for the moment
     */
    mkChatHeader(buff, CHAT_AUTH_REQ, datalen);
    mkSenderHeader(buff, _nick.toUtf8());
    mkAuthReq(buff);

    _attempt = 1;
    ui->msgEdit->setEnabled(true);

    _sock->write(buff, bufflen);

    return 0;
}

int qChat::sndMsg()
{
    if (_ws == CLIENT_AUTHENTICATED) {
        QString data = ui->msgEdit->text();
        displayMsg(_nick, data);
        ui->msgEdit->clear();

        int datalen = 4 + _nick.toUtf8().size() + data.toUtf8().size();
        int bufflen = sizeof(chat_header) +  datalen;
        char buff[bufflen];
        memset(buff, 0, bufflen);

        mkChatHeader(buff, CHAT_DATA, datalen);
        mkSenderHeader(buff, _nick.toUtf8());
        mkChatData(buff, data.toUtf8());

        _sock->write(buff, bufflen);

        return 0;
    }
    return -1;
}

void qChat::try_connect()
{
    _sock->connectToHost(_host, _port);
}

int qChat::displayError(QAbstractSocket::SocketError err)
{
    switch (err) {
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, "Error", "server not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
    case QAbstractSocket::RemoteHostClosedError: {
        int to = _attempt * DELAY;
        if (to < MAX_TO_MS)
            _attempt++;
        else
            to = MAX_TO_MS;
        _connecTimer->start(to);
        ui->msgEdit->setEnabled(false);
        ui->userList->clear();
        ui->textEdit->setTextColor(Qt::blue);
        ui->textEdit->append(tr("* connection to lorenzobianconi.net failed"));
        break;
    }
    default:
        break;
    }

    return 0;
}
