#include "client.h"

 // Width of the progress bar

void updateProgress(off_t current, off_t total) {
    float percentage = (float)current / total * 100;
    int filled = (int)((float)current / total * PROGRESS_WIDTH);
    
    printf("\r");
    
    printf("[");
    for (int i = 0; i < PROGRESS_WIDTH; i++) {
        if (i < filled) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("] %.1f%%", percentage);
    
    if (current == total) {
        printf("\n");
    }
    
    fflush(stdout); 
}

int recvn(int sockfd, void *buf, long total) {
    char *p = (char *)buf;
    long cursize = 0;
    while (cursize < total) {
        ssize_t sret = recv(sockfd, p + cursize, total - cursize, 0);
        if (sret == 0) {
            return 1;
        }
        cursize += sret;
    }
    return 0;
}

int recvWithProgress(int sockfd, char *buf, long total) {
    char *p = (char *)buf;
    long cursize = 0;
    while (cursize < total) {
        ssize_t sret = recv(sockfd, p + cursize, total - cursize, 0);
        if (sret == 0) {
            return 1;
        }
        cursize += sret;
        updateProgress(cursize, total);
    }
    return 0;
}

int recvFile(int sockfd) {
    char filename[4096] = {0};
    train_t train;
    
    // Receive file name
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    memcpy(filename, train.data, train.length);
    
    // Receive file size
    off_t filesize;
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    memcpy(&filesize, train.data, train.length);
    
    printf("Receiving file: %s\n", filename);
    printf("File size: %.2f MB\n", (double)filesize / (1024 * 1024));
    
    // Create and map file
    int fd = open(filename, O_CREAT|O_RDWR|O_TRUNC, 0666);
    ERROR_CHECK(fd, -1, "open");
    
    int ret = ftruncate(fd, filesize);
    ERROR_CHECK(ret, -1, "ftruncate");
    
    char *p = (char*)mmap(NULL, filesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(p, MAP_FAILED, "mmap");
    
    // Use the receive function with a progress bar
    printf("Progress:\n");
    recvWithProgress(sockfd, p, filesize);
    
    // Clean resoure
    munmap(p, filesize);
    close(fd);
    
    printf("File transfer completed successfully!\n");
    return 0;
}

int PathInfoInit(PathInfo *pathinfo){
    memcpy(pathinfo->rootPath,ROOTPATH,sizeof(ROOTPATH));
    memcpy(pathinfo->curPath,pathinfo->rootPath,sizeof(pathinfo->rootPath));
    memset(&pathinfo->commands,0,sizeof(pathinfo->commands));

    stackInit(&pathinfo->stack);
}

int recvTrain(int sockfd, train_t *train){
    ssize_t ret;
    // 接收长度字段
    ret = recv(sockfd, &train->length, sizeof(train->length), 0);
    if (ret <= 0 || ret != sizeof(train->length)) {
        fprintf(stderr, "Failed to receive length\n");
        return -1;
    }
    // 检查数据长度是否合理
    if (train->length > BUFSIZE) {
        fprintf(stderr, "Path length exceeds size\n");
        return -1;
    }
    // 接收数据字段
    ret = recv(sockfd, train->data, train->length, 0);
    if (ret <= 0 || ret != train->length) {
        fprintf(stderr, "Failed to receive data\n");
        return -1;
    }
    return 0;  // 成功
}
void serialize(char *data[], size_t count, char **buffer, size_t *total_size){
    *total_size = 0;

    for(size_t i = 0; i < count; i++){
        *total_size += strlen(data[i]) + 1; 
    }
    *buffer = malloc(*total_size);
    if(*buffer == NULL){
        perror("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    char *ptr = *buffer;
    for (size_t i = 0; i < count; i++){
        size_t len = strlen(data[i]) + 1;
        memcpy(ptr, data[i], len);
        ptr += len;
    }
}

int transFile(int netfd,const char *path){
    int responseCode = SUCCESS;
    train_t train;
    char filePath[4096] = {0};
    memcpy(filePath, path, strlen(path));
    struct stat statbuf;
    int fd = open(filePath,O_RDWR);
    fstat(fd,&statbuf);
    char *filename = basename(strdup(filePath));
    if(fd == -1){
        printf("File does not exist: %s", path);
    }
    
    train.length = strlen(filename);
    memcpy(train.data,filename,train.length);
    send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL);
    recvResponseCode(netfd,&responseCode);
    if(responseCode == PATH_ERROR){
        printf("Upload lose! Path is exist!\n");
        return -1;
    }
    train.length = sizeof(off_t);
    memcpy(train.data,&statbuf.st_size,train.length);
    send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL);
    //sleep(10);
    printf("Done!\n");
    sendfile(netfd,fd,NULL,statbuf.st_size);
    close(fd);
    
    return 0;

}

int getCommand(PathInfo *pathinfo, int netfd){
    printf("args = %s\n",pathinfo->commands.args);
    char path[4096] = {0};
    char cmdCopy[1024] = {0};
    if (!pathinfo || !pathinfo->commands.args) {
        return -1;
    }
    if (strlen(pathinfo->commands.args) >= sizeof(cmdCopy)) {
        return -1;
    }
    strcpy(cmdCopy, pathinfo->commands.args);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL, " /");
    if (!cmd) {
        strcpy(path, "./");
    } else {
        while (cmd != NULL) {
            if (strlen(path) + strlen(cmd) + 2 >= MAX_PATH_LEN) {
                return -1; 
            }
            strcat(path, cmd);
            cmd = strtok(NULL, " /");
            if(cmd != NULL){
            strcat(path, "/");
            }
        }
    }
    printf("path = %s\n",path);
    if(transFile(netfd,path) == -1){
        return -1;
    }
    return 0;
}

int fileCommand(PathInfo *pathinfo, int sockfd) {
    train_t train;
    int responseCode = SUCCESS;
    char response[4096] = {0};

    // 分别发送路径信息
    sendTrain(sockfd, pathinfo->curPath, strlen(pathinfo->curPath));
    sendTrain(sockfd, pathinfo->rootPath, strlen(pathinfo->rootPath));
    //END

    //发送stack.path
    char *buffer = NULL;
    size_t total_size = 0;
    serialize(pathinfo->stack.path,pathinfo->stack.top,&buffer,&total_size);
    send(sockfd,buffer,total_size,0);
    free(buffer);
    //END
    static int isFirstExecution = 1;
    if(isFirstExecution){
    recvResponseCode(sockfd,&responseCode);
        if (recvTrain(sockfd,&train) == -1){
            responseCode = PATH_ERROR;
        }
        memcpy(response,train.data,train.length);
        printf("%s\n",response);
        isFirstExecution = 0;
    }
    fgets(pathinfo->commands.args, sizeof(pathinfo->commands.args), stdin);
    sendTrain(sockfd, pathinfo->commands.args, strlen(pathinfo->commands.args));
    commandAnalyze(pathinfo);
    
    // printf("top = %d\n",pathinfo->stack.top);
    // printf("1 = %s, 2 = %s\n",pathinfo->stack.path[0],pathinfo->stack.path[1]);
    // printf("size %ld\n",strlen(pathinfo->stack.path[0]));
if(responseCode == SUCCESS){
    switch (pathinfo->commands.type) {
        case cd:
            printf("switch to cd\n");
            recvResponseCode(sockfd,&responseCode);
            if (recvTrain(sockfd,&train) == -1){
                responseCode = PATH_ERROR;
            }
            memcpy(response,train.data,train.length);
            printf("Server : %s\n",response);
            if(responseCode == SUCCESS){
                printf("Success!\n");
                char cmdCopy[1024] = {0};
                bzero(cmdCopy, sizeof(cmdCopy));
                strcpy(cmdCopy, pathinfo->commands.args);
                char *cmd = strtok(cmdCopy, " /");
                cmd = strtok(NULL," /");
                while(cmd != NULL){
                    if(strcmp(cmd,"..") == 0){
                        stackPop(&pathinfo->stack);
                        char newPath[MAX_PATH_LEN] = {0};
                        memcpy(newPath,pathinfo->stack.path[0],sizeof(pathinfo->stack.path[0]));
                        for(int i = 1; i < pathinfo->stack.top; i++){
                            strcat(newPath,"/");
                            strcat(newPath,pathinfo->stack.path[i]);
                        }
                        memcpy(pathinfo->curPath,newPath,sizeof(newPath));
                        printf("currpath = %s\n",newPath);
                    }else{
                        stackPush(&pathinfo->stack,cmd);
                        strcat(pathinfo->curPath,"/");
                        strcat(pathinfo->curPath,pathinfo->stack.path[pathinfo->stack.top -1]);
                    }
                cmd = strtok(NULL," /");
            }
            printf("Curpath = %s\n",pathinfo->curPath);
            printf("Rootpath = %s\n",pathinfo->rootPath);
            }
            break;
        case ls:
            char lsResult[4096] = {0};
            recvTrain(sockfd,&train);
            memcpy(lsResult,train.data,train.length);
            printf("%s\n",lsResult);
            break;
        case pwd:
            char pwdPath[MAX_PATH_LEN] = {0}; 
            recvTrain(sockfd,&train);
            memcpy(pwdPath,train.data,train.length);
            printf("%s\n",pwdPath);
            break;
        case put:
            int getret = getCommand(pathinfo,sockfd);
            if(getret == 0){
                printf("Upload Successful !\n");
            }
            break;
        case get:
            printf("switch to get\n");
            recvResponseCode(sockfd,&responseCode);
            if (recvTrain(sockfd,&train) == -1){
                responseCode = PATH_ERROR;
            }
            memcpy(response,train.data,train.length);
            printf("%s\n",response);
            if(responseCode == SUCCESS){
                recvFile(sockfd);
            }
            break;
        case rm:
            recvResponseCode(sockfd,&responseCode);
            if (responseCode == SUCCESS){
                printf("Rmove dir Sucessfully!\n");
            }else if(responseCode == PATH_ERROR){
                printf("Rmove dir Fail!\n");
            }
            break;
        case mk:
            recvResponseCode(sockfd,&responseCode);
            if (responseCode == SUCCESS){
                printf("Make dir sucessfully!\n");
            }else if(responseCode == PATH_ERROR){
                printf("Make dir fail!\n");
            }
            break;
    }
}
    return 0;
}
