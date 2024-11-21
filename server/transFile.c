#include "threadPool.h"


int transFile(int netfd,const char *path){
    char response[4096] = {0};  // 用于存储响应信息
    int responseCode = SUCCESS;
    train_t train;
    char filePath[4096] = {0};
    memcpy(filePath, path, strlen(path));
    struct stat statbuf;
    int fd = open(filePath,O_RDWR);
    fstat(fd,&statbuf);
    char *filename = basename(strdup(filePath));
    if(fd == -1){
        sprintf(response, "File does not exist: %s", path);
        MY_LOG_ERROR("%s\n",response);
        responseCode = PATH_ERROR;
    }else{
        sprintf(response, "Successfully get file: %s", filename);
        MY_LOG_INFO("%s\n",response);
    }
    int ret = sendResponseCode(netfd, responseCode);
    if (ret == -1) {
        MY_LOG_ERROR("send responseCode failed");
    }
    ret = sendTrain(netfd, response, strlen(response));
    if (ret == -1) {
        MY_LOG_ERROR("send response failed");
    }
    
    
    train.length = strlen(filename);
    memcpy(train.data,filename,train.length);
    if(send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL) == -1){
        return -1;
    }
    
    train.length = sizeof(off_t);
    memcpy(train.data,&statbuf.st_size,train.length);
    if(send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL) == -1){
        return -1;
    }
    //sleep(10);
    MY_LOG_INFO("Done!\n");

    if(sendfile(netfd,fd,NULL,statbuf.st_size) == -1){
        return -1;
    }
    close(fd);
    
    return 0;

}
