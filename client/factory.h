#ifndef __FACTORY__
#define __FACTORY__

#include "client.h"
#include "md5.h"
#include "../include/head.h"

#define PROGRESS_WIDTH 50 


int user_command(int sockfd, User *user);
int log_in_command(int sockfd, User *user);
int sign_up_command(int sockfd, User *user);
int sign_out_command(int sockfd, User *user);
int putCommand(int netfd,char *args);
void serialize(char *data[], size_t count, char **buffer, size_t *total_size);
int transFile(int netfd,const char *path);
void updateProgress(off_t current, off_t total);
int recvn(int sockfd, void *buf, long total);
int recvWithProgress(int sockfd, char *buf, long total);
int recvFile(int sockfd);
int Compute_file_md5(const char *file_path, char *md5_str);
int put_command(int netfd,char *args);



#endif