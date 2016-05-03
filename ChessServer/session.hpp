#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>


class Session {
private:
	int sockfd;
	bool admin;

public:
	Session(const int new_sockfd);

	bool get_permissions() const;
	void set_permissions(const bool isAdmin);
	int get_sockfd();
};


Session::Session(const int new_sockfd) {
	sockfd = new_sockfd;
	admin = false;
}

bool Session::get_permissions() const {
	return admin;
}

void Session::set_permissions(const bool isAdmin) {
	admin = isAdmin;
}

int Session::get_sockfd(){
	return sockfd;
}