#include "worker.h"
#include "taskQueue.h"
#include "threadPool.h"

int exitPipe[2];
void handler(int signum){
    printf("signum = %d\n",signum);
    write(exitPipe[1],"1",1);
}
int main(int argc, char *argv[]){
    ARGS_CHECK(argc,4);
    log_init("Server start", LOG_LOCAL0, MY_LOG_LEVEL_DEBUG);
    pipe(exitPipe);
    if(fork() != 0){
        close(exitPipe[0]);
        signal(SIGUSR1,handler);
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            MY_LOG_ERROR("Child exited normally with code %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            MY_LOG_ERROR("Child terminated by signal %d\n", WTERMSIG(status));
        } else {
            MY_LOG_ERROR("Child terminated abnormally\n");
        }
        wait(0);
        MY_LOG_INFO("Parent is going to exit!\n");
        exit(0);
    }

    close(exitPipe[1]);
    threadPool_t threadPool;
    threadPoolInit(&threadPool,atoi(argv[3]));
    makeWorker(&threadPool);

    int sockfd;
    tcpInit(argv[1],argv[2],&sockfd);
    int epfd = epoll_create(1);
    epollAdd(epfd,sockfd);
    epollAdd(epfd,exitPipe[0]);
    while(1){
        struct epoll_event readySet[1024];
        int readyNum = epoll_wait(epfd,readySet,1024,-1);
        for(int i = 0; i < readyNum; ++i){
            if(readySet[i].data.fd == sockfd){
                int netfd = accept(sockfd,NULL,NULL);
                MY_LOG_INFO("One task pending\n");
                pthread_mutex_lock(&threadPool.mutex);
                enQueue(&threadPool.taskQueue,netfd);
                pthread_cond_signal(&threadPool.cond);
                MY_LOG_INFO("One task assigned\n");
                pthread_mutex_unlock(&threadPool.mutex);
            }else if(readySet[i].data.fd == exitPipe[0]){
                MY_LOG_INFO("ThreadPool is going to exit\n");
                pthread_mutex_lock(&threadPool.mutex);
                threadPool.exitFlag = 1;
                pthread_cond_broadcast(&threadPool.cond);
                pthread_mutex_unlock(&threadPool.mutex);
                for(int j = 0; j < threadPool.tidArr.workerNum; ++j){
                    pthread_join(threadPool.tidArr.arr[j],NULL);
                }
                MY_LOG_INFO("Main thread is going to exit\n");
                exit(0);
            }
        }
    }
    log_close();
    return 0;
}
