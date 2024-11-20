#ifndef __WORKER__
#define __WORKER__
#include "../include/head.h"

typedef struct tidArr_s{
    pthread_t *arr;
    int workerNum;
}tidArr_t;
int tidArrInit(tidArr_t *ptidArr, int workerNum);
void *threadFunc(void *arg);
int clientCommand(int netfd);
void checkCommand(PathInfo *pathinfo, int netfd);
int cdCommand(PathInfo *pathinfo, int netfd);
int lsCommand(PathInfo *pathinfo, int netfd);
int pwdCommand(PathInfo *pathinfo, int netfd);
int getCommand(PathInfo *pathinfo, int netfd);
int putCommand(int netfd);
int rmCommand(PathInfo *pathinfo, int netfd);
int mkCommand(PathInfo *pathinfo, int netfd);
void deserialize(char *buffer, size_t total_size, char ***data, size_t *count);
int recvFile(int sockfd);
#endif
