#ifndef __FACTORY__
#define __FACTORY__

#include "client.h"

int user_command(int sockfd, User *user);
int log_in_command(int sockfd, User *user);
int sign_up_command(int sockfd, User *user);
int sign_out_command(int sockfd, User *user);


#endif