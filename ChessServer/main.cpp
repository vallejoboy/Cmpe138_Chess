#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // usleep
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <mysql++.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <openssl/sha.h>
#include "session.hpp"

//global variables
mysqlpp::Connection sql_connection;
std::string sql_db = "chess", sql_host = "localhost", sql_user = "chess", sql_pass = "gamer123";
bool quit = false;

//Catchs exit signal to close connections before program closes
void sig_handler(const int signum) { 
	puts("\nGot interrupt signal."); 
	if (signum == SIGINT)
	{
        //Closes all connections before exit
		printf("SIGINT Received, Closing\n");
		quit = true;
	}
}

//struct for connections to store connection user-level
struct connections{ // Todo for server side admin-style
    int sockfd;
    bool admin;
};


//sha256
void sha256(char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

//handles cleaned message from client
//void handle_message(char *buffer, const int length, int sockfd, Session session, const int sessionID){
void handle_message(char *buffer, const int length, Session *session) { 
    //lowercases everything
    for (int i = 0; i < length-1 ; i++){
        buffer[i] = tolower(buffer[i]);
    }

    // Counts how many words there are in string
    uint32_t count = 1;   
    for (uint32_t x = 0; x < length; ++x) { 
        if (buffer[x] == ' ') count++;  // Total of spaces + 1  = # of words
    } 

    char *data = (char*)malloc(length+1); // Allocates memory for copy of message
    char **arg = (char**)malloc(sizeof(char*)*(count)); // Allocates 1 pointer per word
    strcpy(data, buffer); // copies message into data
    
    //Splits up cstring by replacing spaces with newlines, also sets pointer for each word
    int last = 0;
    count = 0;
    for (uint32_t x = 0; x < length; ++x) {  
        if (data[x] == ' ') {
            data[x] = '\0';
            arg[count] = &data[last];
            last = x + 1;
            count++;
        }
    }
    arg[count] = &data[last];
    count++;

    //Shows Arguments
    for (int test = 0; test < count ; test++){
        if (test == 2 && strcmp (arg[0] , "login") == 0){
        std::cout << "Paramter [" << test+1 << "] :Password Hidden" << std::endl;    
        }
        else
        std::cout << "Paramter [" << test+1 << "] :" << arg[test] << std::endl;
    }

	char queryBuffer[5000]; //Buffer to store query results in

 

    //Query Commands 
    if ( strcmp( arg[0] , "query") == 0 && (count == 2 || count == 3)){
    	const char *none_found = "No entries found!";

        //Sample Test Query
        if ( strcmp (arg[1] , "test") == 0 && count == 2){
    		testquery:
    		int bufferPos = 0;
    		mysqlpp::Query name_query = sql_connection.query();
    		name_query << "SELECT * FROM users WHERE wins > 10";
    		try {
    			mysqlpp::StoreQueryResult result = name_query.store();
                if (result.num_rows() == 0){
                    write(session->get_sockfd(), none_found, strlen(none_found)+1);
                    goto end;
                }
    			bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer) - bufferPos, "%15s %8s %8s\n", "name", "games", "wins");

    			for (int x = 0; x < result.num_rows(); x++) {
    				bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer)-bufferPos,"%15s %8i %8i\n", std::string(result[x]["name"]).c_str(), (int)result[x]["games"], (int)result[x]["wins"]);
    			}
    			write(session->get_sockfd(), (void*)queryBuffer, bufferPos+1);
    		}
    		 catch (mysqlpp::BadQuery queryErr) { //Should not happen?
                char errBuffer[2048];
                snprintf(errBuffer, sizeof(errBuffer), "SQL reported bad query: %s. Attempting reconnect...", sql_connection.error());
                puts(errBuffer);
                std::cerr << "Query error: " << queryErr.errnum() << std::endl;
                write(session->get_sockfd(), "Query Error. Try Again", 23);

            }
            catch (mysqlpp::ConnectionFailed badCon){
                     try {
                        sql_connection.connect(sql_db.c_str(), sql_host.c_str(), sql_user.c_str(), sql_pass.c_str(), 3307);
                        puts("Successfully reconnected to SQL server.");
                        goto testquery;
                    }
                    catch (mysqlpp::ConnectionFailed sql_error) {
                        std::string error = "SQL reconnect error: "; 
                        error += sql_error.what();
                        puts(error.c_str());
                        write(session->get_sockfd(), "Connection Error", 17);
                    }
            }
        }
        //End of Test Query

        //User paramter query
        else if ( strcmp (arg[1] , "stats") == 0 && count == 3){  // query stats "name"
            test2query:
            int bufferPos = 0;
            mysqlpp::Query name_query = sql_connection.query();
            name_query << "SELECT * FROM users WHERE name = "  << mysqlpp::quote << arg[2]; // Sends Query
            try {
                mysqlpp::StoreQueryResult result = name_query.store();
                if (result.num_rows() == 0){
                    write(session->get_sockfd(), none_found, strlen(none_found)+1);
                    goto end;
                }
                bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer) - bufferPos, "%15s %8s %8s\n", "name", "games", "wins");
                for (int x = 0; x < result.num_rows(); x++) {
                    bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer)-bufferPos,"%15s %8i %8i\n", std::string(result[x]["name"]).c_str(), (int)result[x]["games"], (int)result[x]["wins"]);
                }
                write(session->get_sockfd(), (void*)queryBuffer, bufferPos+1);
            }
            catch (mysqlpp::BadQuery queryErr) { //Should not happen?
                char errBuffer[2048];
                snprintf(errBuffer, sizeof(errBuffer), "SQL reported bad query: %s. Attempting reconnect...", sql_connection.error());
                puts(errBuffer);
                std::cerr << "Query error: " << queryErr.errnum() << std::endl;
                write(session->get_sockfd(), "Query Error. Try Again", 23);
            }
            catch (mysqlpp::ConnectionFailed badCon){
                     try {
                        sql_connection.connect(sql_db.c_str(), sql_host.c_str(), sql_user.c_str(), sql_pass.c_str(), 3307);
                        puts("Successfully reconnected to SQL server.");
                        goto test2query;
                    }
                    catch (mysqlpp::ConnectionFailed sql_error) {
                        std::string error = "SQL reconnect error: "; 
                        error += sql_error.what();
                        puts(error.c_str());
                        write(session->get_sockfd(), "Connection Error", 17);
                    }
            }

        }   

       

        //If query + 1-2 words that dont fit
        else {
            write(session->get_sockfd(), "Invalid query paramters! Type \"help\" for help.", 49);
        }
    }

    //help command
    else if ( strcmp (arg[0] , "help") == 0 && count == 1){
    	const char *help_message = "Help Menu:\n"
								   "help -- opens help menu\n"
								   "login -- prompts for admin user and password to login\n"
                                   "query stats username -- returns stats of that username\n"
                                   "query gamenumber gameid -- return stats of that gameid\n"
                                   "query top10 -- returns leaderboard highest 10 ELO's\n"                               
								   "quit -- quits program";
 		//send data to socket here
 		write(session->get_sockfd(), (void*)help_message, strlen(help_message)+1);
    }

   
    //admin login command w/ sha256  
    else if (strcmp (arg[0] , "login") == 0 && count == 3){
        static char sha256key[65];
        sha256(arg[2], sha256key);
        //chessadmin/cmpe138danny
        if ((strcmp (arg[1] , "chessadmin") == 0 && strcmp (sha256key , "409c7307b6fc6bae4aa41a56ca9603505f1e07d90b800bd08dcb7b6093a05bae") == 0 )){
    		const char *login_auth = "Login Sucessful!";
            //set admin flag for connection
            session->set_permissions(true);
    		write(session->get_sockfd(), (void*)login_auth, strlen(login_auth) + 1);             //send data to socket here
        }
        else {
            write(session->get_sockfd(), "Invalid login!", 15);
        }
	}

    else if (strcmp (arg[0] , "logout") == 0 && count == 1){
        //check if user was admin
        //set admin flag off
        session->set_permissions(false);
        //session[x].set_permissions(false);
        write(session->get_sockfd(), "Logged Out!", 12);
    }


    //admin insert
    else if (strcmp (arg[0] , "insert") == 0 && count == 3){
        //check if user is admin
        if (session->get_permissions() == false){
            write(session->get_sockfd(), "You are not an admin!", 23);
            return;
        }

        //plaintext admin stuff to mysql
        //arg[1] = plaintext mysql command
        mysqlpp::Query insert_query = sql_connection.query();

        char rawInsert[strlen(arg[2])];
        std::cout << "Size: "<< sizeof(rawInsert) << std::endl;
        strcpy(rawInsert, arg[2]);

        //Testing purposes
        //std::cout << "R1: " << arg[2] << " vs " << rawInsert << std::endl;
        //Removes Quotations
        for (int i = 0; i < strlen(rawInsert); i++){
        rawInsert[i] = rawInsert[i+1];
        }
        rawInsert[strlen(rawInsert)-1] = '\0';
        //Testing Purposes
        //std::cout << "R2: " << arg[2] << " vs " << rawInsert << std::endl;

        try{
            //std::cout << "Okay" << std::endl;
            std::cout << "INSERT INTO "<< arg[1] << " VALUES " << rawInsert << ";" << std::endl;

            insert_query << "INSERT INTO "<< arg[1] << " VALUES " << rawInsert << ";";
            insert_query.execute();
        }
        catch (mysqlpp::BadQuery er) { // handle any connection or
        // query errors that may come up
        std::cerr << "Error: " << er.what() << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        } catch (const mysqlpp::BadConversion& er) {
        // Handle bad conversions
        std::cerr << "Conversion error: " << er.what() << std::endl <<
                "\tretrieved data size: " << er.retrieved <<
                ", actual size: " << er.actual_size << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        } catch (const mysqlpp::Exception& er) {
        // Catch-all for any other MySQL++ exceptions
        std::cerr << "Error: " << er.what() << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        }
        write(session->get_sockfd(), "Insert Sucess!", 15);
    }

    //admin update

    else if ( strcmp (arg[0] , "update") == 0 && (count == 3 || count == 4)){
        //check if user is admin
        if (session->get_permissions() == false){
            write(session->get_sockfd(), "You are not an admin!", 23);
            return;
        }

        mysqlpp::Query update_query = sql_connection.query();

        char rawUpdate[strlen(arg[2])];
        strcpy(rawUpdate, arg[2]);
        char rawUpdate2[strlen(arg[3])];
        strcpy(rawUpdate2, arg[3]);

        //Testing purposes
        std::cout << "R1: " << arg[2] << " vs " << rawUpdate << std::endl;
        //Removes Quotations
        for (int i = 0; i < strlen(rawUpdate); i++){
        rawUpdate[i] = rawUpdate[i+1];
        }
        rawUpdate[strlen(rawUpdate)-1] = '\0';
        //Testing Purposes
        std::cout << "R2: " << arg[2] << " vs " << rawUpdate << std::endl;

        if (count == 4){
            //Testing purposes
            //std::cout << "R1: " << arg[2] << " vs " << rawUpdate2 << std::endl;
            //Removes Quotations
            for (int i = 0; i < strlen(rawUpdate2); i++){
            rawUpdate2[i] = rawUpdate2[i+1];
            }
            rawUpdate2[strlen(rawUpdate2)-1] = '\0';
            //Testing Purposes
            //std::cout << "R2: " << arg[2] << " vs " << rawUpdate2 << std::endl;
        }


        try{
            if (count == 4){ // W/ where
               // std::cout << "Okay" << std::endl;
                std::cout << "UPDATE "<< arg[1] << " SET " << rawUpdate << " WHERE " << rawUpdate2 << ";" << std::endl;
                update_query << "UPDATE "<< arg[1] << " SET " << rawUpdate << " WHERE " << rawUpdate2 <<  ";";
            }
            else if (count == 3){ // W/o where
                //std::cout << "Okay" << std::endl;
                std::cout << "UPDATE "<< arg[1] << " SET " << ";" << std::endl;
                update_query << "UPDATE "<< arg[1] << " SET " << ";";
            }
            update_query.execute();
        }
        catch (mysqlpp::BadQuery er) { // handle any connection or
        // query errors that may come up
        std::cerr << "Error: " << er.what() << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        } catch (const mysqlpp::BadConversion& er) {
        // Handle bad conversions
        std::cerr << "Conversion error: " << er.what() << std::endl <<
                "\tretrieved data size: " << er.retrieved <<
                ", actual size: " << er.actual_size << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        } catch (const mysqlpp::Exception& er) {
        // Catch-all for any other MySQL++ exceptions
        std::cerr << "Error: " << er.what() << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        }
        write(session->get_sockfd(), "Update Sucess!", 15);
    }

    //admin delete
    else if ( strcmp (arg[0] , "delete") == 0 && (count == 2 || count == 3)){
        //check if user is admin
        if (session->get_permissions() == false){
            write(session->get_sockfd(), "You are not an admin!", 23);
            return;
        }

        mysqlpp::Query delete_query = sql_connection.query();

        char rawDelete[strlen(arg[2])];
        strcpy(rawDelete, arg[2]);

        if (count == 3){
            //Testing purposes
            //std::cout << "R1: " << arg[2] << " vs " << rawDelete << std::endl;
            //Removes Quotations
            for (int i = 0; i < strlen(rawDelete); i++){
            rawDelete[i] = rawDelete[i+1];
            }
            rawDelete[strlen(rawDelete)-1] = '\0';
            //Testing Purposes
            //std::cout << "R2: " << arg[2] << " vs " << rawDelete << std::endl;
         }
        try{
            if (count == 3){
                //std::cout << "Okay" << std::endl;
                std::cout << "DELETE FROM "<< arg[1] << " where " << rawDelete << ";" << std::endl;
                delete_query << "DELETE FROM "<< arg[1] << " where " << rawDelete << ";";
            }
            else if (count == 2){
                //std::cout << "Okay" << std::endl;
                std::cout << "DELETE FROM "<< arg[1] << ";" << std::endl;
                delete_query << "DELETE FROM "<< arg[1] << ";";
            }
            delete_query.execute();
        }
        catch (mysqlpp::BadQuery er) { // handle any connection or
        // query errors that may come up
        std::cerr << "Error: " << er.what() << std::endl;
        write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        } catch (const mysqlpp::BadConversion& er) {
        // Handle bad conversions
        std::cerr << "Conversion error: " << er.what() << std::endl <<
                "\tretrieved data size: " << er.retrieved <<
                ", actual size: " << er.actual_size << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;

        } catch (const mysqlpp::Exception& er) {
        // Catch-all for any other MySQL++ exceptions
        std::cerr << "Error: " << er.what() << std::endl;
            write(session->get_sockfd(), "Error syntax/bad values!", 25);
            goto end;
        }
        write(session->get_sockfd(), "Delete Sucess!", 15);
    }

    //bad command
	else
	{
		write(session->get_sockfd(), "Invalid command! Type \"help\" for help.", 39);
	}

    end:
    free(data);
    free (arg);
    return;
}

int main() {
    //Sig Handler Error Check
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");

    //Set up server side socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    Session *connections = NULL;
    int n_connections = 0;
   
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(4000); // using port 4000
   
    if (bind(tcp_socket, (struct sockaddr*)&server_address, sizeof(server_address))) {
        puts("Call to bind failed.");
        return -1;
    }
    if (listen(tcp_socket, 128)) {
        puts("Call to listen failed.");
        return -2;
    }
    //End server side socket setup

    // Connect to our sql db here.
   	sql_connection = mysqlpp::Connection(mysqlpp::use_exceptions);
	sql_connection.connect(sql_db.c_str(), sql_host.c_str(), sql_user.c_str(), sql_pass.c_str(), 3307);

    //Server loop waiting for connections and messages
    while (!quit) {
        int conn;
        // Accept new connections
        do {
            conn = accept(tcp_socket, (struct sockaddr*)NULL, NULL);
            if (conn >= 0) {
                
            	puts("Got a new connection");
                Session *ptr = (Session*)malloc(sizeof(Session)*(n_connections+1));
                if (ptr) {
                    memcpy(ptr, connections, sizeof(Session)*n_connections);
                    ptr[n_connections] = Session(conn);
                    free(connections);
                    connections = ptr;
                    n_connections++;
                }
            }
        } while (conn > 0);
        
        // Reading from socket into buffer
        char buffer[2000];
        for (int x = 0; x < n_connections; ++x) {
            const int len = recv(connections[x].get_sockfd(), (void*)buffer, sizeof(buffer)-1, MSG_DONTWAIT);
			const int err = errno;
			if (errno != 11) printf("%i: %s\n", errno, strerror(err));
            if (len > 0) { // We got a message
                buffer[len] = 0;
            	printf("Got msg: %s\n", buffer);
                handle_message(buffer, len, &connections[x]);
            } else if (len == 0) { // Our client disconnected
            	printf("Client %i disconnected\n",x);
            	if (n_connections > 1) connections[x] = connections[n_connections-1];
                n_connections--;
            }
        }
        // Sleep for 10ms so we don't use more cpu time than necessary.
        usleep(10000);
    }


    free(connections);
    close(tcp_socket);
}

