#include <QDateTime>
#include <QDir>
#include <QHostAddress>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>


#include "client.h"
#include "qversareserver.h"
#include "utils.h"

QVersareServer::QVersareServer(QObject *parent, QCoreApplication *app,
                               ServerSettings *settings, QSqlDatabase *ddbb) :
    QTcpServer(parent),mydb_(ddbb,app,settings->getDbName(),settings->getDaemon())
{
    settings_ = settings;
    //Register metatype for queue QVERSOS in the msg loop
    qRegisterMetaType<QVERSO>("QVERSO");
    daemonMode_ = settings->getDaemon();

}

QVersareServer::~QVersareServer()
{
    for(QMap<qintptr,QPointer<Client>>::Iterator i = clients_.begin();
        i != clients_.end(); ++i) {
        i.value()->die();
    }
}

void QVersareServer::startServer()
{

    if(!this->listen( QHostAddress(settings_->getIpAddress()),
                      settings_->getPort()) )
        helperDebug(daemonMode_,"Could not start server");
    else
        helperDebug(daemonMode_,"Listening...");
}


void QVersareServer::incomingConnection(qintptr handle)
{
    //Comprobar que no se han eliminado los ficheros de clave y cert
    QDir filesDir;
    QString rutaPem = "/etc/qVersareServer/qVersareServer.pem";
    if(filesDir.exists(rutaPem)) {
        QPointer<Client> clientSocket = new Client(handle,daemonMode_,
                                                   rutaPem,
                                                   rutaPem,
                                                   this);
        clients_.insert(clients_.end(),handle,clientSocket);
        //threads with parents are not movable

        if (!clientSocket->waitForEncryption()){
            helperDebug(daemonMode_, "No se pudo hacer el handhsake");
            clientSocket->die();
        } else {
            clientSocket->makeConnections(this);
            clientSocket->setParent(0);
            clientSocket->start();
        }
    } else {
        helperDebug(daemonMode_, "Verificar ficheros .key y .crt");
    }

}

void QVersareServer::newMessageFromClient(QVERSO aVerso,Client *fd)
{
    //Good moment for store the message in the database
    QString room = QString::fromStdString(aVerso.room());
    QString username = QString::fromStdString(aVerso.username());
    QString message = QString::fromStdString(aVerso.message());

    mydb_.addMessage(room,username, message);

    emit forwardedMessage(aVerso,fd);
}

void QVersareServer::clientDisconnected(int fd)
{
    QPointer<Client> temp = clients_.take(fd);
    delete temp;
}

void QVersareServer::validateClient(QString user, QString password,
                                    Client *whoClient)
{
    emit validateResult(mydb_.goodCredentials(user,password),whoClient);
}

void QVersareServer::newInTheRoom(QString room, Client *fd)
{
    //Emitimos los 10 ultimos mensajess para el usuario conectado
    QList<QVERSO> lastMessages = mydb_.getLastTenMessages(room);
    QListIterator<QVERSO> it(lastMessages);
    while(it.hasNext()) {
        emit messageFromHistory(it.next(),fd);
    }

    //Emitimos los timestamps de los avatares de los demás al usuario
    QListIterator<Client*> i(clientsPerRoom_.value(room));
    QList<QString> clientsNames;
    while(i.hasNext())
        clientsNames.append(i.next()->getName());
    QList<QVERSO> otherUsersTimeStamp = mydb_.getOthersUsersTimestamps(clientsNames);


    QListIterator<QVERSO> it2(otherUsersTimeStamp);
    while(it2.hasNext()) {
        emit userTimeStamp(it.next(), fd);
    }
    //Añadimos al usuario a la lista
    helperDebug(daemonMode_, "User added to room: " + room);
    addClientToList(room, fd);
}

void QVersareServer::removeMeFromRoom(QString room, Client *fd)
{
    removeClientFromList(room, fd);
}

void QVersareServer::updateClientAvatar(QString user, QString avatar,
                                        QDateTime timestamp)
{
    mydb_.updateClientAvatar(user, avatar, timestamp);
}



void QVersareServer::setupDatabase()
{
    //Create table for users
    QSqlQuery query(mydb_);
    query.exec("CREATE TABLE IF NOT EXISTS users ("
                  "USERNAME VARCHAR(60) PRIMARY KEY,"
                  "PASSWORD VARCHAR(40),"
                  "AVATAR BLOB,"
                  "AVTIMESTAMP INTEGER)");
    //Create table for msgs
    query.exec("CREATE TABLE IF NOT EXISTS messages ("
               "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
               "ROOM VARCHAR(60),"
               "USERNAME VARCHAR(60),"
               "MESSAGE VARCHAR(2000),"
               "TIMESTAMP INTEGER)");

    //Insert basic users
    query.prepare("INSERT ON CONFLICT IGNORE INTO users (username,password)"
                  "VALUES (:username, :password)");
    query.bindValue(":username","pepito");
    query.bindValue(":password","50648aff18d36a6b89cb7dcda2e4e8c5");
    query.bindValue(":avatar","null");
    query.bindValue(":avtimestamp",0);
    query.exec();

    query.prepare("INSERT ON CONFLICT IGNORE INTO users (username,password)"
                  "VALUES (:username, :password)");
    query.bindValue(":username","tiger");
    query.bindValue(":password","9e95f6d797987b7da0fb293a760fe57e");
    query.bindValue(":avatar","null");
    query.bindValue(":avtimestamp",0);
    query.exec();

    query.prepare("INSERT ON CONFLICT IGNORE INTO users (username,password)"
                  "VALUES (:username, :password)");
    query.bindValue(":username","qversare");
    query.bindValue(":password","3b867c3941a04ab062bba35d8a69a1d9");
    query.bindValue(":avatar","null");
    query.bindValue(":avtimestamp",0);

    query.exec();
}

void QVersareServer::addMessage(QString room, QString username, QString message)
{
    QSqlQuery query(mydb_);
    query.prepare("INSERT INTO messages (room,username,message)"
                  "VALUES (:room, :username, :message)");
    query.bindValue(":room",room);
    query.bindValue(":username",username);
    query.bindValue(":message",message);
    query.exec();
}

QList<QVERSO> QVersareServer::getLastTenMessages(QString room)
{
    QList<QVERSO>aux;
    //Query for extract the messages from the ddbb
    //The list comes on desc type!!
    QSqlQuery query(mydb_);
    query.prepare("SELECT * FROM messages WHERE room=(:ROOM) ORDER BY id "
                  "desc limit 10");
    query.bindValue(":ROOM",room);
    query.exec();

    while(query.next()) {
        QVERSO tempVerso;
        tempVerso.set_username(query.value("username").toString().toStdString());
        tempVerso.set_room(room.toStdString());
        tempVerso.set_message(query.value("message").toString().toStdString());
        aux.push_front(tempVerso);
    }

    return aux;

}

QList<QVERSO> QVersareServer::getOthersUsersTimestamps(QString room)
{
    emit userAvatar(mydb_.getThisUserAvatar(user), fd);
}

void QVersareServer::onRequestedTimestamp(QString user, Client *fd)
{
    emit userTimeStamp(mydb_.getThisUserTimeStamp(user), fd);
}

void QVersareServer::addClientToList(QString room, Client *client)
{
    QList<Client*> aux;
    if(clientsPerRoom_.contains(room)) {
        aux = clientsPerRoom_.take(room);
        aux.append(client);
        clientsPerRoom_.insert(room, aux);
    } else {
        aux.append(client);
        clientsPerRoom_.insert(room, aux);
    }
}

void QVersareServer::removeClientFromList(QString room, Client *client)
{
    QList<Client*> aux;
    aux = clientsPerRoom_.take(room);
    aux.removeOne(client);
    clientsPerRoom_.insert(room, aux);
}
