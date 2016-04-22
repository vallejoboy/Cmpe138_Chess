#include "mytcpsocket.h"

MyTcpSocket::MyTcpSocket(QObject *parent) :
    QObject(parent)
{
}

void MyTcpSocket::doConnect()
{
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

    qDebug() << "connecting...";

    // this is not blocking call
    socket->connectToHost("sniperdad.com", 4001);

    // we need to wait...
    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
    }
}

void MyTcpSocket::write(const char* message){
    qDebug() << "Sending";
    qint64 check = socket->write(message);
    qDebug() << "Bytes Sent: " << check; // Should print -1 if error otherwise bytes sent
    const int err = errno;
    printf("%s\n", strerror(err));
}

void MyTcpSocket::close(){
    socket->close();
}

void MyTcpSocket::connected()
{
    qDebug() << "connected...";

    // Hey server, tell me about you.
    socket->write("HEAD / HTTP/1.0\r\n\r\n\r\n\r\n");
}

void MyTcpSocket::disconnected()
{
    qDebug() << "disconnected...";
}

void MyTcpSocket::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " bytes written...";
}

void MyTcpSocket::readyRead()
{
    qDebug() << "reading...";

    // read the data from the socket
    qDebug() << socket->readAll();
}
