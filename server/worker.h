#ifndef __WORKER__
#define __WORKER__
#include "../include/head.h"
#include "log.h"
#include "factory.h"
#include "sql.h"
#include <mysql/mysql.h>
#include <ctype.h>

#define SERVER_ROOT_PATH "/home/joel/linux2/netdisk/netdisk/server"
#define ERROR_CHECK(ret,num,msg) {if(ret==num){MY_LOG_ERROR(msg); return -1;}}

int tidArrInit(tidArr_t *ptidArr, int workerNum);
void *threadFunc(void *arg);
int userCommand(int netfd,User *user, MYSQL *mysql);
//int clientCommand(int netfd);
int log_in_command(int netfd,User *user, MYSQL *mysql);
int sign_up_command(int netfd,User *user, MYSQL *mysql);
int sign_out_command(int netfd,User *user, MYSQL *mysql);
int clientCommand(int netfd,User *user,MYSQL *mysql);
int cdCommand(int netfd,User *user,char *args,MYSQL *mysql);
int lsCommand(int netfd,User *user,MYSQL *mysql);



// void checkCommand(PathInfo *pathinfo, int netfd);
// int cdCommand(PathInfo *pathinfo, int netfd);
// int lsCommand(PathInfo *pathinfo, int netfd);
// int pwdCommand(PathInfo *pathinfo, int netfd);
// int getCommand(PathInfo *pathinfo, int netfd);
// int putCommand(int netfd);
// int rmCommand(PathInfo *pathinfo, int netfd);
// int mkCommand(PathInfo *pathinfo, int netfd);
// void deserialize(char *buffer, size_t total_size, char ***data, size_t *count);
// int recvFile(int sockfd);
// int checkEnter(User user);

#endif
