#ifndef __TCP__
#define __TCP__
#include "../include/head.h"
#include "worker.h"


typedef struct node_s{
    int netfd;
    struct node_s * pNext;
}node_t;

typedef struct tidArr_s{
    pthread_t *arr;
    int workerNum;
}tidArr_t;

typedef struct taskQueue_s{
    node_t *pFront;
    node_t *pRear;
    int queueSize;
}taskQueue_t;

typedef struct threadPool_s{
    tidArr_t tidArr;
    taskQueue_t taskQueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int exitFlag;
}threadPool_t;

int taskQueueInit(taskQueue_t *pqueue);
int enQueue(taskQueue_t *pqueue, int netfd);
int deQueue(taskQueue_t *pqueue);

int threadPoolInit(threadPool_t *threadPool, int workerNum);
int makeWorker(threadPool_t *pthreadPool);
int tcpInit(const char *ip, const char *port, int *psockfd);
int epollAdd(int epfd, int fd);
int epollDel(int epfd, int fd);


int transFile(int netfd,const char *path);

#endif