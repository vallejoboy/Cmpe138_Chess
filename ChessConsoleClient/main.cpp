#include <QCoreApplication>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QThread>
#include <windows.h>

using namespace std;


void echo( bool on = true ){
  DWORD  mode;
  HANDLE hConIn = GetStdHandle( STD_INPUT_HANDLE );
  GetConsoleMode( hConIn, &mode );
  mode = on
       ? (mode |   ENABLE_ECHO_INPUT )
       : (mode & ~(ENABLE_ECHO_INPUT));
  SetConsoleMode( hConIn, mode );
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    string name, password, command, temp = "login approved";
    bool quit = false, admin = false;
    char buffer[2000];

    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost("sniperdad.com", 4000);

    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
        cout << "Exiting Program";
        return a.exec();
    }
    else{
        cout << "Connected to server!" << endl;
    }
    cout << "Welcome Guest, type \"help\" for a list of commands\n";

    while(!quit){
        //Test Admin Stuff, Probably move to server side
        if (admin == false) {
            cout << "<Guest>";
        }
        else {
            cout << "<Admin>";
        }
        //End Test admin Stuff

        cin >> command;

        if (command == "quit"){
            quit = true;
            break;
        }
        if (command == "login"){
            cout << "Enter your username:";
            cin >> name;
            cout << "Enter your password:";
            echo(false);
            cin >> password;
            echo(true);
            command = "login "+ name + " " + password;
            cout << endl;
        }

        socket->write(command.c_str());
        socket->flush();
        socket->waitForReadyRead(-1);
        socket->read(buffer, sizeof(buffer));
        cout << buffer << "\n";

        //Test Admin Stuff, Probably move to server side

        if (strcmp( buffer , temp.c_str()) == 0){
            admin = true;
        }
        //End Test admin Stuff

    }

    socket->close();
    return a.exec();
}


