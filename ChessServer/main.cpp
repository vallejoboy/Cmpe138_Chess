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

mysqlpp::Connection sql_connection;
std::string sql_db = "chess", sql_host = "localhost", sql_user = "chess", sql_pass = "gamer123";
bool quit = false;

void sig_handler(const int signum) { //Catchs Signals
	puts("\nGot interrupt signal."); // line break
	if (signum == SIGINT)
	{
		printf("SIGINT Received, Closing\n");
		quit = true;
	}
}

struct connections{ // Todo for server side admin-style
    int sockfd;
    bool admin;
};


void handle_message(char *buffer, const int length, int sockfd) {
	//printf("Check: %s is %i\n",buffer,strcmp(buffer, "help"));

	char queryBuffer[5000]; //Buffer to store query results in

    for (int i = 0; i < length-1 ; i++){
    	buffer[i] = tolower(buffer[i]);
    }

    if ( strcmp( buffer, "query") == 0){
    	//do sql associated with this
		//send(sockfd, "Temp", 5, 0);
 		//send data to socket here
 		//send(sockfd, (void*)msg, length, 0);
		startquery:
		int bufferPos = 0;
		mysqlpp::Query name_query = sql_connection.query();
		name_query << "SELECT * FROM users WHERE wins > 10";
		try {
			mysqlpp::StoreQueryResult result = name_query.store();
			bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer) - bufferPos, "%15s %8s %8s\n", "name", "games", "wins");
			for (int x = 0; x < result.num_rows(); x++) {
				bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer)-bufferPos,"%15s %8i %8i\n", std::string(result[x]["name"]).c_str(), (int)result[x]["games"], (int)result[x]["wins"]);
			}
			write(sockfd, (void*)queryBuffer, bufferPos+1);
		}
		catch (mysqlpp::BadQuery error) {
			char errBuffer[2048];
			snprintf(errBuffer, sizeof(errBuffer), "SQL reported bad query: %s. Attempting reconnect...", sql_connection.error());
			puts(errBuffer);
			try {
				sql_connection.connect(sql_db.c_str(), sql_host.c_str(), sql_user.c_str(), sql_pass.c_str(), 3307);
				puts("Successfully reconnected to SQL server.");
				goto startquery;
			}
			catch (mysqlpp::ConnectionFailed sql_error) {
				std::string error = "SQL reconnect error: "; error += sql_error.what();
				puts(error.c_str());
				write(sockfd, "Query Error. Try Again", 23);
				return;
			}
		}
    }

    else if ( strcmp( buffer, "help") == 0){
    	const char *help_message = "Help Menu:\n"
								   "help -- opens help menu\n"
								   "login -- prompts for admin user and password to login\n"
                                   "query stats username -- returns stats of that username\n"
                                   "query gamenumber gameid -- return stats of that gameid\n"
                                   "query top10 -- returns leaderboard highest 10 ELO's\n";                               
								   "quit -- quits program";
 		//send data to socket here
 		write(sockfd, (void*)help_message, strlen(help_message)+1);
    }

	else if (strcmp(buffer, "login danny gamer123") == 0) {
		const char *login_auth = "login approved";
		//send data to socket here
		write(sockfd, (void*)login_auth, strlen(login_auth) + 1);
	}

	else
	{
		write(sockfd, "Invalid command! Type \"help\" for help.", 39);
	}

}
int main() {
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");
    int tcp_socket = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
    int *connections = NULL;
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

    // Connect to our sql db here.
   	sql_connection = mysqlpp::Connection(mysqlpp::use_exceptions);
    //sql_connection.connect("chess", "localhost", "chess", "gamer123", 3306);
	sql_connection.connect(sql_db.c_str(), sql_host.c_str(), sql_user.c_str(), sql_pass.c_str(), 3307);

    
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

    for (int z = 0; z < n_connections; ++z){
       close(connections[z]);
    }
    close(tcp_socket);


		
}

