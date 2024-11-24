#ifndef __SQL__
#define __SQL__

#include "worker.h"
#include <mysql/mysql.h>

int sql_connect(MYSQL **conn);
int search_user(const char *name,char *salt,char *crypt_pw, MYSQL *mysql);
int creat_user(const char *name,char *salt,char *crypt_pw, MYSQL *mysql);
char *generate_salt();
int out_user(const char *name,MYSQL *mysql);
int search_user_up(const char *name,MYSQL *mysql);
int pathInit(User *user,MYSQL *mysql);
int check_dir(User *user, const char *path,int top,MYSQL *mysql);
int first_dir_init(User *user,MYSQL *mysql);
int ls_dir(User *user,char *path,MYSQL *mysql);

#endif