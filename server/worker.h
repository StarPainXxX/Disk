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
int log_in_command(int netfd,User *user, MYSQL *mysql);
int sign_up_command(int netfd,User *user, MYSQL *mysql);
int sign_out_command(int netfd,User *user, MYSQL *mysql);
int clientCommand(int netfd,User *user,MYSQL *mysql);
int cdCommand(int netfd,User *user,char *args,MYSQL *mysql);
int lsCommand(int netfd,User *user,MYSQL *mysql);
int pwdCommand(int netfd,User *user);
int mkCommand(int netfd,User *user,char *args,MYSQL *mysql);
int rmCommand(int netfd,User *user,char *args,MYSQL *mysql);
int putCommand(int netfd,User *user,MYSQL *mysql);
int getCommand(int netfd,User *user,MYSQL *mysql);
#endif
