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
    int responseCode = SUCCESS;
    train_t train;
    char *buffer = NULL;
    recvResponseCode(sockfd,&responseCode);
    if(responseCode == PATH_ERROR){
        printf("The file does not exist!\n");\
        return 0;
    }
    
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,&train.data,train.length);
    memcpy(filename,train.data,train.length);
    int fd = open(filename,O_RDWR);
    if(fd == -1){
        responseCode = PATH_NOT_EXIST;
        sendResponseCode(sockfd,responseCode);
        off_t filesize;
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        memcpy(&filesize,train.data,train.length);
        printf("Receiving file: %s\n", filename);
        printf("File size: %.2f MB\n", (double)filesize / (1024 * 1024));
        fd = open(filename,O_CREAT|O_RDWR|O_TRUNC,0666);
        buffer = malloc(filesize);
        recvWithProgress(sockfd,buffer,filesize);
        if(write(fd, buffer, filesize) != filesize){
            printf("Write file error!\n");
            free(buffer);
            close(fd);
            return -1;
        }
        free(buffer);
        close(fd);
    }else{
        responseCode = PATH_EXIST;
        sendResponseCode(sockfd,responseCode);
        struct stat st;
        train.length = sizeof(off_t);
        memcpy(train.data,&st.st_size,train.length);
        send(sockfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL);
        recvResponseCode(sockfd,&responseCode);
        if(responseCode == PATH_EXIST){
            printf("The file already exists!\n");
            close(fd);
            return 0;
        }else if(responseCode = PATH_NOT_EXIST){
            printf("The file is incomplete. Prepare to accept the remaining file contents\n");
            off_t filesize;
            recvn(sockfd,&train.length,sizeof(train.length));
            recvn(sockfd,train.data,train.length);
            memcpy(&filesize,train.data,train.length);
            printf("Receiving file: %s\n", filename);
            printf("File size: %.2f MB\n", (double)filesize / (1024 * 1024));
            lseek(fd,0,SEEK_END);
            buffer = malloc(filesize);
            recvWithProgress(sockfd,buffer,filesize);
            if(write(fd, buffer, filesize) != filesize){
                printf("Write file error!\n");
                free(buffer);
                close(fd);
                return -1;
            }
            free(buffer);
            close(fd);
        }
    }
    responseCode = SUCCESS;
    sendResponseCode(sockfd,responseCode);
    printf("Successfully!\n");
    return 0;
}

// int PathInfoInit(PathInfo *pathinfo){
//     memcpy(pathinfo->rootPath,ROOTPATH,sizeof(ROOTPATH));
//     memcpy(pathinfo->curPath,pathinfo->rootPath,sizeof(pathinfo->rootPath));
//     memset(&pathinfo->commands,0,sizeof(pathinfo->commands));

//     stackInit(&pathinfo->stack);
// }

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

int transFile(int netfd, const char *path) {
    int responseCode = SUCCESS;
    train_t train;
    char filePath[4096] = {0};
    // 安全地复制路径
    strncpy(filePath, path, sizeof(filePath) - 1);
    
    struct stat statbuf;
    int fd = open(filePath, O_RDWR);
    if (fd == -1) {
        perror("File open error");
        return -1;
    }
    // 获取文件信息
    if (fstat(fd, &statbuf) == -1) {
        perror("fstat error");
        close(fd);
        return -1;
    }
    // 获取文件名
    char *filename = basename(filePath);    
    // 发送文件名
    train.length = strlen(filename);
    printf("filename %s\n",filename);
    memcpy(train.data, filename, train.length);
    if (send(netfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL) == -1) {
        perror("send filename failed");
        close(fd);
        return -1;
    }
    // 接收响应码
    if (recvResponseCode(netfd, &responseCode) == -1) {
        perror("recv response code failed");
        close(fd);
        return -1;
    }
    // 定义大文件阈值 100M
    const off_t LARGE_FILE_THRESHOLD = 100 * 1024 * 1024;  // 100MB
    if (responseCode == PATH_EXIST) {
        // 接收已传输的文件大小
        off_t st_size = 0;
        if(recv(netfd,&train.length,sizeof(train.length),0) == -1){
            close(fd);
            return -1;
        }
        if(recv(netfd,&train.data,train.length,0) == -1){
            close(fd);
            return -1;
        }
        
        memcpy(&st_size, train.data, sizeof(off_t));
        // 检查是否需要续传
        if (st_size == statbuf.st_size) {
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
            printf("Upload lose! Path is exist!\n");
            close(fd);
            return -1;
        }
        // 从断点处继续传输
        if (lseek(fd, st_size, SEEK_SET) == -1) {
            perror("lseek failed");
            close(fd);
            return -1;
        }
        memset(&train,0,sizeof(train));
        // 发送剩余文件大小
        train.length = sizeof(off_t);
        off_t s_size = statbuf.st_size - st_size;
        printf("%ld\n",s_size);
        char buffer[sizeof(off_t)];
        memcpy(train.data, &s_size, sizeof(off_t));
        printf("trainlenth = %s\n",train.data);
        if (send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL) == -1) {
            perror("send file size failed");
            close(fd);
            return -1;
        }
        
        // 大文件使用 mmap
        if (statbuf.st_size > LARGE_FILE_THRESHOLD) {
            void *mapped = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (mapped == MAP_FAILED) {
                perror("mmap failed");
                close(fd);
                return -1;
            }
            
            // 发送剩余文件内容
            ssize_t sent = send(netfd, (char*)mapped + st_size, statbuf.st_size - st_size, MSG_NOSIGNAL);
            if (sent == -1) {
                perror("send mmap content failed");
                munmap(mapped, statbuf.st_size);
                close(fd);
                return -1;
            }
            // 取消内存映射
            munmap(mapped, statbuf.st_size);
        } else {
            // 小文件使用 sendfile
            if (sendfile(netfd, fd, NULL, statbuf.st_size - st_size) == -1) {
                perror("sendfile failed");
                close(fd);
                return -1;
            }
        }
    } else if(responseCode == PATH_ERROR){
        printf("It's a directory Can't trans\n");
        return -1;
    }else if(responseCode == PATH_NOT_EXIST){
        // 发送总文件大小
        train.length = sizeof(off_t);
        memcpy(train.data, &statbuf.st_size, train.length);
        if (send(netfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL) == -1) {
            perror("send file size failed");
            close(fd);
            return -1;
        }
        // 大文件使用 mmap
        if (statbuf.st_size > LARGE_FILE_THRESHOLD) {
            void *mapped = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (mapped == MAP_FAILED) {
                perror("mmap failed");
                close(fd);
                return -1;
            }
            // 发送整个文件内容
            ssize_t sent = send(netfd, mapped, statbuf.st_size, MSG_NOSIGNAL);
            if (sent == -1) {
                perror("send mmap content failed");
                munmap(mapped, statbuf.st_size);
                close(fd);
                return -1;
            }
            // 取消内存映射
            munmap(mapped, statbuf.st_size);
        } else {
            // 小文件使用 sendfile
            if (sendfile(netfd, fd, NULL, statbuf.st_size) == -1) {
                perror("sendfile failed");
                close(fd);
                return -1;
            }
        }
    }
    close(fd);
    printf("File transfer Done!\n");
    return 0;
}

int fileCommand(int sockfd){
    train_t train;
    int responseCode = SUCCESS;
    Command commands;
    fgets(commands.args,sizeof(commands.args),stdin);
    sendTrain(sockfd,commands.args,strlen(commands.args));
    commandAnalyze(&commands);

    switch (commands.type)
    {
    case cd:
        recvResponseCode(sockfd,&responseCode);
        if(responseCode == PATH_ERROR){
            printf("Unable to go past the root directory!\n");
        }else if(responseCode == PATH_NOT_EXIST){
            printf("Path not exist\n");
        }
        break;
    case ls:
        memset(&train,0,sizeof(train));
        
        recvResponseCode(sockfd,&responseCode);
        if(responseCode == PATH_ERROR){
            printf("Folder is empty!\n");
        }else{
            char path[MAX_LS_LEN] = {0};
            recvTrain(sockfd,&train);
            memcpy(path,train.data,train.length);
            printf("%s\n",path);
        }
        break;
    case pwd:
        char path[MAX_PATH_LEN] = {0};
        recvTrain(sockfd,&train);
        memcpy(path,train.data,train.length);
        printf("%s\n",path);
        break;
    case mk:
        recvResponseCode(sockfd,&responseCode);
        if(responseCode == PATH_NOT_EXIST){
            printf("Please enter a valid path!\n");
        }else if(responseCode == PATH_EXIST){
            printf("Folder already exists!\n");
        }else{
            printf("Created successfully!\n");
        }
        break;
    case rm:
        recvResponseCode(sockfd,&responseCode);
        if(responseCode == SUCCESS){
            printf("Remove file successfully!\n");
        }else{
            printf("The floder is not empty!\n");
        }
        break;
    case -1:
        printf("Invalid command!\n");
        break;
    case quit:
        return -1;
    }
}




