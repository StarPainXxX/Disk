#ifndef __CLIENT__
#define __CLIENT__

#include "../include/head.h"
#include "factory.h"

#define PROGRESS_WIDTH 50 


void updateProgress(off_t current, off_t total);
int recvn(int sockfd, void *buf, long total);
int recvWithProgress(int sockfd, char *buf, long total);
int recvFile(int sockfd);

int fileCommand(int sockfd);
void serialize(char *data[], size_t count, char **buffer, size_t *total_size);
int transFile(int netfd,const char *path);
#endif