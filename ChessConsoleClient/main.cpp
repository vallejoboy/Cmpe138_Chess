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

//Disables echo for password
void echo( bool on = true ){
  DWORD  mode;
  HANDLE hConIn = GetStdHandle( STD_INPUT_HANDLE );
  GetConsoleMode( hConIn, &mode );
  mode = on
       ? (mode |   ENABLE_ECHO_INPUT )
       : (mode & ~(ENABLE_ECHO_INPUT));
  SetConsoleMode( hConIn, mode );
}

//Eats newline from fgets
void chomp(char *s) {
    while(*s && *s != '\n') s++;
    *s = 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //Declare variables here
    string temp = "Login Sucessful!";
    bool quit = false, admin = false;
    char buffer[2000], command[2000], user[100], password[100];

    //TCP Connection
    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost("sniperdad.com", 4001);

    //Checks for connection
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

        //Grabs user input
        fseek(stdin,0,SEEK_END); //Resets stdin to beginning
        fgets(command,sizeof(command),stdin); // Grabs whole line of command
        chomp(command); // Removes newline from command

        //Client side commands
        if (strcmp (command , "quit") == 0){
            quit = true;
            break;
        }
        else if (strcmp (command , "login") == 0){
            cout << "Enter your username:";
            cin >> user;
            cout << "Enter your password:";
            echo(false);
            cin >> password;
            echo(true);
            strcpy(command,"login ");
            strcat(command,user);
            strcat(command," ");
            strcat(command,password);
            cout << endl;
        }

//      cout << ":" << command << ":" << endl; //Test Stuff: Shows what were sending to socket

        //sends data to socket and waits for response
        socket->write(command);
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


