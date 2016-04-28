#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // usleep
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mysql++.h>
#include <iostream>
#include <signal.h>

//global variables
mysqlpp::Connection sql_connection;
std::string sql_db = "chess", sql_host = "localhost", sql_user = "chess", sql_pass = "gamer123";
bool quit = false;

//Catchs exit signal to close connections before program closes
void sig_handler(const int signum) { 
	puts("\nGot interrupt signal."); 
	if (signum == SIGINT)
	{
		printf("SIGINT Received, Closing\n");
		quit = true;
	}
}

//struct for connections to store connection user-level
struct connections{ // Todo for server side admin-style
    int sockfd;
    bool admin;
};

//handles cleaned message from client
void handle_message(char *buffer, const int length, int sockfd) { 
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
                    write(sockfd, none_found, strlen(none_found)+1);
                    return;
                }
    			bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer) - bufferPos, "%15s %8s %8s\n", "name", "games", "wins");

    			for (int x = 0; x < result.num_rows(); x++) {
    				bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer)-bufferPos,"%15s %8i %8i\n", std::string(result[x]["name"]).c_str(), (int)result[x]["games"], (int)result[x]["wins"]);
    			}
    			write(sockfd, (void*)queryBuffer, bufferPos+1);
    		}
    		 catch (mysqlpp::BadQuery queryErr) { //Should not happen?
                char errBuffer[2048];
                snprintf(errBuffer, sizeof(errBuffer), "SQL reported bad query: %s. Attempting reconnect...", sql_connection.error());
                puts(errBuffer);
                std::cerr << "Query error: " << queryErr.errnum() << std::endl;
                write(sockfd, "Query Error. Try Again", 23);
                return;
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
                        write(sockfd, "Connection Error", 17);
                        return;
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
                    write(sockfd, none_found, strlen(none_found)+1);
                    return;
                }
                bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer) - bufferPos, "%15s %8s %8s\n", "name", "games", "wins");
                for (int x = 0; x < result.num_rows(); x++) {
                    bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer)-bufferPos,"%15s %8i %8i\n", std::string(result[x]["name"]).c_str(), (int)result[x]["games"], (int)result[x]["wins"]);
                }
                write(sockfd, (void*)queryBuffer, bufferPos+1);
                return;
            }
            catch (mysqlpp::BadQuery queryErr) { //Should not happen?
                char errBuffer[2048];
                snprintf(errBuffer, sizeof(errBuffer), "SQL reported bad query: %s. Attempting reconnect...", sql_connection.error());
                puts(errBuffer);
                std::cerr << "Query error: " << queryErr.errnum() << std::endl;
                write(sockfd, "Query Error. Try Again", 23);
                return;
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
                        write(sockfd, "Connection Error", 17);
                        return;
                    }
            }

        }   

        //If query + 1-2 words that dont fit
        else {
            write(sockfd, "Invalid query paramters! Type \"help\" for help.", 49);
            return;
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
 		write(sockfd, (void*)help_message, strlen(help_message)+1);
    }

    //admin login command
    else if (strcmp (arg[0] , "login") == 0 && count == 3){
        if ((strcmp (arg[1] , "danny") == 0 && strcmp (arg[2] , "gamer123") == 0 )){
    		const char *login_auth = "Login Sucessful!";
    		//send data to socket here
    		write(sockfd, (void*)login_auth, strlen(login_auth) + 1);
        }
        else {
            write(sockfd, "Invalid login!", 15);
        }
	}

    //bad command
	else
	{
		write(sockfd, "Invalid command! Type \"help\" for help.", 39);
	}

}

int main() {
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");

    //Set up server side socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    int *connections = NULL;
    int n_connections = 0;
   
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(4001); // using port 4000
   
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
                int *ptr = (int*)malloc(sizeof(int)*(n_connections+1));
                if (ptr) {
                    memcpy(ptr, connections, sizeof(int)*n_connections);
                    ptr[n_connections] = conn;
                    free(connections);
                    connections = ptr;
                    n_connections++;
                }
            }
        } while (conn > 0);
        // Reading from socket into buffer
        char buffer[2000];
        for (int x = 0; x < n_connections; ++x) {
            const int len = recv(connections[x], (void*)buffer, sizeof(buffer)-1, MSG_DONTWAIT);
			const int err = errno;
			if (errno != 11) printf("%i: %s\n", errno, strerror(err));
            if (len > 0) { // We got a message
                buffer[len] = 0;
            	printf("Got msg: %s\n", buffer);
                handle_message(buffer, len, connections[x]);
            } else if (len == 0) { // Our client disconnected
            	printf("Client %i disconnected\n",x);
            	if (n_connections > 1) connections[x] = connections[n_connections-1];
                n_connections--;
            }
        }
        // Sleep for 10ms so we don't use more cpu time than necessary.
        usleep(10000);
    }

    //Closes all connections before exit
    for (int z = 0; z < n_connections; ++z){
       close(connections[z]);
    }
    close(tcp_socket);


		
}
