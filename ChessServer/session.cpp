#include "session.hpp"

Session::Session(const int new_sockfd) {
	sockfd = new_sockfd;
	user_permissions = false;
}

bool Session::get_permissions() const {
	return admin;
}

void Session::set_permissions(const bool isAdmin) {
	admin = isAdmin;
}
