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

mysqlpp::Connection sql_connection;


void handle_message(char *buffer, const int length, int sockfd) {
	//printf("Check: %s is %i\n",buffer,strcmp(buffer, "help"));

	char queryBuffer[5000];
	//int bufferPos = 0;

    for (int i = 0; i < length-1 ; i++){
    	buffer[i] = tolower(buffer[i]);
    }

    if ( strcmp( buffer, "query") == 0){
    	//do sql associated with this
		//send(sockfd, "Temp", 5, 0);
 		//send data to socket here
 		//send(sockfd, (void*)msg, length, 0);

		int bufferPos = 0;
		mysqlpp::Query name_query = sql_connection.query();
		name_query << "SELECT * FROM users WHERE wins > 10";
		try {
			mysqlpp::StoreQueryResult result = name_query.store();
			bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer) - bufferPos, "%15s %8s %8s\n", "name", "games", "wins");
			for (int x = 0; x < result.num_rows(); x++) {
				bufferPos += snprintf(&queryBuffer[bufferPos], sizeof(queryBuffer)-bufferPos,"%15s %8i %8i\n", std::string(result[x]["name"]).c_str(), (int)result[x]["games"], (int)result[x]["wins"]);
			}
			send(sockfd, (void*)queryBuffer, bufferPos+1, 0);
		}
		catch (mysqlpp::BadQuery error) {
			send(sockfd, "Not a valid query", 18, 0);
		}
    }

    else if ( strcmp( buffer, "help") == 0){
    	const char *help_message = "Help Menu:\n"
								   "query stats [name] -- returns stats of that user\n"
								   "help -- opens help menu\n"
								   "login admin -- prompts for admin password to login\n"
								   "quit -- quits program";
 		//send data to socket here
 		send(sockfd, (void*)help_message, strlen(help_message)+1, 0);
    }

	else if (strcmp(buffer, "login admin") == 0) {
		const char *help_message2 = "admin login";
		//send data to socket here
		send(sockfd, (void*)help_message2, strlen(help_message2) + 1, 0);
	}

	else
	{
		send(sockfd, "Invalid command! Type \"help\" for help.", 39, 0);
	}

}
int main() {
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
    sql_connection.connect("chess", "localhost", "chess", "gamer123", 3306); // todo 

	//virtual bool 	connect (const char *db=0, const char *server=0, const char *user=0, const char *password=0, unsigned int port=0)


    bool quit = false;
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
   	
   
}