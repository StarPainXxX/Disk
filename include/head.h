#ifndef __HEAD__
#define __HEAD__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <libgen.h>
#include <errno.h>
#include<crypt.h>
#include<shadow.h>
#include <sys/syslog.h>
#include <stdarg.h>

#define ARGS_CHECK(argc,num) {if(argc!=num){fprintf(stderr,"args error!\n");return -1;}}
#define ERROR_CHECK(ret,num,msg) {if(ret==num){perror(msg); return -1;}}
#define THREAD_ERROR_CHECK(ret,msg) {if(ret != 0){fprintf(stderr,"%s:%s\n",msg,strerror(ret));}}

#define MAX_PATH_LEN 256
#define MAX_STACK_LEN 256
#define MAX_COMMAND_LEN 256
#define MAX_USER_NAME 256
#define MAX_PASSWARD 256
#define BUFSIZE 4096

#define SUCCESS 0
#define PATH_ERROR -1
#define PERMISSION_ERROR -2
#define USER_ERROR -3
#define PASSWARD_ERROR -4





typedef enum{
    cd,
    ls,
    pwd,
    put,
    get,
    rm,
    mk
}CommandType;

typedef struct{
    CommandType type;
    char args[MAX_COMMAND_LEN];
    char path[MAX_PATH_LEN];
}Command;

typedef struct{
    char *path[MAX_STACK_LEN];
    int top;
    int capacity;
}PathStack;

typedef struct{
    char curPath[MAX_PATH_LEN];
    char rootPath[MAX_PATH_LEN];
    Command commands;
    PathStack stack;
}PathInfo;

typedef struct{
    int length;        // locomotive
    char data[BUFSIZE];   // train car
}train_t;

typedef struct{
    char UserName[MAX_USER_NAME];
    char Passward[MAX_PASSWARD];
}User;


int stackInit(PathStack *stack);
int stackPush(PathStack *stack, const char *path);
int stackPop(PathStack *stack);
CommandType getCommandType(const char *cmd);
int sendTrain(int sockfd, const char *data, int length);
int recvTrain(int sockfd, train_t *train);
int recvn(int sockfd, void *buf, long total);
int commandAnalyze(PathInfo *pathinfo);
int sendResponseCode(int netfd, int responseCode);
int recvResponseCode(int netfd, int *responseCode);
int recvInt(int sock, int *value);
int sendInt(int netfd, int value);
#endif