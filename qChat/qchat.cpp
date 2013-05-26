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

    /* server parameters */
    _host = "lorenet.dyndns.org";
    _port = 9999;
    _nick = getHostname() + QString("@") + QHostInfo::localHostName();
    _sock = new QTcpSocket(this);

    _ws = CLIENT_NOT_AUTHENTICATED;

    tableFormat.setBorder(0);

    connect(ui->msgEdit, SIGNAL(returnPressed()), this, SLOT(sndMsg()));
    connect(_sock, SIGNAL(connected()), this, SLOT(clientAuth()));
    connect(_sock, SIGNAL(readyRead()), this, SLOT(getMsg()));
    connect(_sock, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    _sock->connectToHost(_host, _port);
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

int qChat::mkChatHeader(char *msg, enum chat_msg type, int msglen)
{
    struct chat_header *ch = (struct chat_header *) msg;

    ch->type = qToBigEndian((int)type);
    ch->dlen = qToBigEndian(msglen);

    return 0;
}

int qChat::mkSenderHeader(char *msg, QString nick)
{
    char *sinfo = (char *)(msg + sizeof(struct chat_header));

    *((int *)sinfo) = qToBigEndian(nick.size());
    memcpy(sinfo + 4, nick.toLatin1().data(), nick.size());

    return 0;
}

int qChat::mkChatData(char *msg, QString text)
{
    char *sInfo = (char *)(msg + sizeof(struct chat_header));
    char *data = (char *)(sInfo + 4 + _nick.size());

    memcpy(data, text.toLatin1().data(), text.size());

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
    struct chat_header *ch = (struct chat_header *) msg;
    char *tmp, *end, *sInfo = (char *)(ch + 1);

    tmp = sInfo + qFromBigEndian(*((int *)sInfo)) + 4;
    end = sInfo + qFromBigEndian(ch->dlen) + 1;

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

    struct chat_header *ch = (struct chat_header *) buff;
    char *sInfo = (char *)(ch + 1);
    int nicklen = qFromBigEndian(*((int *)sInfo));
    char *data = (char *)(sInfo + 4 + nicklen);

    switch (qFromBigEndian((int)ch->type)) {
    case CHAT_AUTH_REP: {
        struct chat_auth_rep *res = (struct chat_auth_rep *)data;

        if (qFromBigEndian((int) res->res_type) == AUTH_SUCCESS)
            _ws = CLIENT_AUTHENTICATED;
        else {
            QMessageBox::information(this, "Error", "Authentication failed");
            this->close();
        }
        break;
    }
    case CHAT_DATA: {
        displayMsg(QString(sInfo + 4).mid(0, nicklen), QString(data));
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
    int datalen = 4 + _nick.size() + sizeof(struct chat_auth_req);
    int bufflen = sizeof(struct chat_header) + datalen + 1;
    char *buff = (char *) malloc(bufflen);
    if (!buff) {
        qDebug() << "buffer allocation failed";
        return -1;
    }
    memset(buff, 0, bufflen);
    /*
     * XXX: open authentication for the moment
     */
    mkChatHeader(buff, CHAT_AUTH_REQ, datalen);
    mkSenderHeader(buff, _nick);
    mkAuthReq(buff);

    _sock->write(buff, bufflen);

    return 0;
}

int qChat::sndMsg()
{
    if (_ws == CLIENT_AUTHENTICATED) {
        QString data = ui->msgEdit->text();
        displayMsg(_nick, data);
        ui->msgEdit->clear();

        int datalen = 4 + _nick.size() + data.size();
        int bufflen = sizeof(chat_header) +  datalen + 1;
        char *buff = (char *) malloc(bufflen);
        if (!buff) {
            qDebug() << "buffer allocation failed";
            return -1;
        }
        memset(buff, 0, bufflen);

        mkChatHeader(buff, CHAT_DATA, datalen);
        mkSenderHeader(buff, _nick);
        mkChatData(buff, data);

        _sock->write(buff, bufflen);

        return 0;
    }
    return -1;
}

int qChat::displayError(QAbstractSocket::SocketError err)
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
    this->close();

    return 0;
}
