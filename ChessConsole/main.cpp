#include <QCoreApplication>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QThread>

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    string name;
    string password;
    bool quit = false;
    string command;
    char buffer[2000];

    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost("sniperdad.com", 4000);

    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
    }
    else{
        cout << "Connected to server!" << endl;
    }
    cout << "Welcome Guest, type \"help\" for a list of commands\n>";

    while(!quit){
        cin >> command;
        if (command == "quit"){
            quit = true;
            break;
        }
        socket->write(command.c_str());
        socket->flush();
        socket->waitForReadyRead(-1);
        socket->read(buffer, sizeof(buffer));
        cout << buffer << "\n>";
    }

    socket->close();

    return a.exec();
}


