#ifndef __THREADPOOL__
#define __THREADPOOL__

#include <cfunc.h>
#include "worker.h"
#include "taskQueue.h"
typedef struct threadPool_s{
    tidArr_t tidArr;
    taskQueue_t taskQueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int exitFlag;
}threadPool_t;

int threadPoolInit(threadPool_t *threadPool, int workerNum);
int makeWorker(threadPool_t *pthreadPool);
int tcpInit(const char *ip, const char *port, int *psockfd);
int epollAdd(int epfd, int fd);
int epollDel(int epfd, int fd);
int transFile(int netfd,const char *path);

#endif
