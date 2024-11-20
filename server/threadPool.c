#include "taskQueue.h"
#include "worker.h"
#include "threadPool.h"
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

