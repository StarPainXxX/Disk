#ifndef __CLIENT__
#define __CLIENT__

#include "../include/head.h"

#define PROGRESS_WIDTH 50 
#define ROOTPATH "Disk/User1"

int PathInfoInit(PathInfo *pathinfo);
void updateProgress(off_t current, off_t total);
int recvn(int sockfd, void *buf, long total);
int recvWithProgress(int sockfd, char *buf, long total);
int recvFile(int sockfd);
int fileCommand(PathInfo *pathinfo,int sockfd);
void serialize(char *data[], size_t count, char **buffer, size_t *total_size);
int transFile(int netfd,const char *path);
#endif