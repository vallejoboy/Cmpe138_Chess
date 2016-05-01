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


