#include "factory.h"

int taskQueueInit(taskQueue_t *pqueue){
    bzero(pqueue,sizeof(taskQueue_t));
    return 0;
}

int enQueue(taskQueue_t *pqueue, int netfd){
    node_t *pNew = (node_t *)calloc(1,sizeof(node_t));
    pNew->netfd = netfd;
    if(pqueue->queueSize == 0){
        pqueue->pFront = pNew;
        pqueue->pRear = pNew;
    }else{
        pqueue->pRear->pNext = pNew;
        pqueue->pRear = pNew;
    }
    ++pqueue->queueSize;
    return 0;
}

int deQueue(taskQueue_t *pqueue){
    node_t *pCur = pqueue->pFront;
    pqueue->pFront = pCur->pNext;
    if(pqueue->queueSize == 1){
        pqueue->pRear == NULL;
    }
    free(pCur);
    --pqueue->queueSize;
    return 0;
}

int threadPoolInit(threadPool_t *pthreadPool, int workerNum){
    tidArrInit(&pthreadPool->tidArr, workerNum);
    taskQueueInit(&pthreadPool->taskQueue);
    pthread_mutex_init(&pthreadPool->mutex,NULL);
    pthread_cond_init(&pthreadPool->cond,NULL);
    pthreadPool->exitFlag = 0;
    return 0;
}

int makeWorker(threadPool_t *pthreadPool){
    for(int i = 0; i < pthreadPool->tidArr.workerNum; ++i){
        pthread_create(&pthreadPool->tidArr.arr[i], NULL,threadFunc, pthreadPool);
    }
    return 0;
}

int tcpInit(const char *ip,const char* port, int *psockfd){
    *psockfd = socket(AF_INET,SOCK_STREAM,0);
    int reuse = 1;
    int ret = setsockopt(*psockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    ERROR_CHECK(ret,-1,"setsockopt");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);
    ret = bind(*psockfd,(struct sockaddr *)&addr,sizeof(addr));
    ERROR_CHECK(ret,-1,"bind");
    listen(*psockfd,50);
    return 0;
}

int epollAdd(int epfd, int fd){
    struct epoll_event events;
    events.events = EPOLLIN;
    events.data.fd = fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&events);
    return 0;
}

int epollDel(int epfd, int fd){
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
    return 0;
}

