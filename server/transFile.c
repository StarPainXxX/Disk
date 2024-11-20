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
        responseCode = PATH_ERROR;
    }else{
        sprintf(response, "Successfully get file: %s", filename);
    }
    int ret = sendResponseCode(netfd, responseCode);
    if (ret == -1) {
        perror("send responseCode failed");
    }
    ret = sendTrain(netfd, response, strlen(response));
    if (ret == -1) {
        perror("send response failed");
    }
    
    
    train.length = strlen(filename);
    memcpy(train.data,filename,train.length);
    send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL);
    
    train.length = sizeof(off_t);
    memcpy(train.data,&statbuf.st_size,train.length);
    send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL);
    //sleep(10);
    printf("Done!\n");
    sendfile(netfd,fd,NULL,statbuf.st_size);
    close(fd);
    
    return 0;

}
