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
int check_dir(int userId, const char *path,int parent_id,MYSQL *mysql);
int first_dir_init(User *user,MYSQL *mysql);
int ls_dir(User *user,char *path,int parent_id,MYSQL *mysql);
int get_dir_id(const char *path,int userId,MYSQL *mysql);
int create_dir(int userId,PathStack *stack,char *name,int parent_id,MYSQL *mysql);
int get_root_id(User *user,MYSQL *mysql);
int rm_dir(char *path,User *user,MYSQL *mysql);
#endif