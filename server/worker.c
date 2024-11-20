#include "worker.h"
#include "threadPool.h"
#include "taskQueue.h"

int tidArrInit(tidArr_t *ptidArr, int workerNum){
    ptidArr->arr = (pthread_t *)calloc(workerNum,sizeof(pthread_t));
    ptidArr->workerNum = workerNum;
    return 0;
}

void unlock(void *arg){
    threadPool_t *pthreadPool = (threadPool_t *)arg;
    printf("Unlock!\n");
    pthread_mutex_unlock(&pthreadPool->mutex);
}

void *threadFunc(void *arg){
    threadPool_t *pthreadPool = (threadPool_t *)arg;
    while(1){
        pthread_mutex_lock(&pthreadPool->mutex);
        int netfd;
        while(pthreadPool->exitFlag == 0 && pthreadPool->taskQueue.queueSize <= 0){
            pthread_cond_wait(&pthreadPool->cond,&pthreadPool->mutex);
        }
        if(pthreadPool->exitFlag == 1){
            printf("One child going to exit\n");
            pthread_mutex_unlock(&pthreadPool->mutex);
            pthread_exit(NULL);
        }
        netfd = pthreadPool->taskQueue.pFront->netfd;
        deQueue(&pthreadPool->taskQueue);
        pthread_mutex_unlock(&pthreadPool->mutex);
        
        //transFile(netfd);
        



        while(1){
        clientCommand(netfd);
        }
        close(netfd);
    }
}

void deserialize(char *buffer, size_t total_size, char ***data, size_t *count){
    *count = 0;
    for(size_t i = 0; i < total_size; i++){
        if(buffer[i] == '\0')(*count)++;
    }
    *data = malloc((*count) * sizeof(char*));
    if(*data == NULL){
        perror("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    char *ptr = buffer;
    for(size_t i = 0; i < *count; i++){
        (*data)[i] = ptr;
        ptr += strlen(ptr)+1;
    }
}



void checkCommand(PathInfo *pathinfo, int netfd) {
    char response[4096] = {0};  // 用于存储响应信息
    int responseCode = SUCCESS;
    struct stat st;
    
        // 1. 验证目标路径
        if (stat(pathinfo->rootPath, &st) == -1) {
            sprintf(response, "User does not exist: %s", pathinfo->rootPath);
            responseCode = PATH_ERROR;
            goto END;
        }
        // 2. 检查是否为目录
        if (!S_ISDIR(st.st_mode)) {
            sprintf(response, "Not a User: %s", pathinfo->rootPath);
            responseCode = PATH_ERROR;
            goto END;
        }
        // 3. 检查访问权限
        if (access(pathinfo->rootPath, X_OK) == -1) {
            sprintf(response, "Permission denied: %s", pathinfo->rootPath);
            responseCode = PERMISSION_ERROR;
            goto END;
        }
        // 4. 切换目录
        if (chdir(pathinfo->rootPath) == -1) {
            sprintf(response, "Failed to change directory: %s", "-1");
            responseCode = PATH_ERROR;
            goto END;
        }
        // 5. 获取当前工作目录（用于确认和返回）
        char currentPath[MAX_PATH_LEN] = {0};
        if (getcwd(currentPath, MAX_PATH_LEN) == NULL) {
            sprintf(response, "Failed to get current directory");
            responseCode = PATH_ERROR;
            goto END;
        }
        // 将标志位设置为0，表示后续执行不再是第一次
        
    
    sprintf(response, "Successfully log in, %s", "Enjoy!");
    
END:
    
    int ret = sendResponseCode(netfd, responseCode);
    if (ret == -1) {
        perror("send responseCode failed");
    }
    ret = sendTrain(netfd, response, strlen(response));
    if (ret == -1) {
        perror("send response failed");
    }
}

int cdCommand(PathInfo *pathinfo, int netfd){
    char response[4096] = {0};  // 用于存储响应信息
    int responseCode = SUCCESS;
    struct stat st;
    char destPath[MAX_PATH_LEN] = {0};
    char cmdCopy[1024] = {0};
    bzero(cmdCopy, sizeof(cmdCopy));
    strcpy(cmdCopy, pathinfo->commands.args);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL," /");
    while(cmd != NULL){
        if(strcmp(cmd,"..") == 0){
            char newPath[MAX_PATH_LEN] = {0};
            strcat(destPath,"../");
            stackPop(&pathinfo->stack);
            for(int i = 1; i < pathinfo->stack.top; i++){
                printf("stack.path[%d] = %s\n",i,pathinfo->stack.path[i]);
                strcat(newPath,"/");
                strcat(newPath,pathinfo->stack.path[i]);
            }
            printf("newpath = %s\n",newPath);
            memcpy(pathinfo->curPath,newPath,sizeof(newPath));
        }else{
            stackPush(&pathinfo->stack,cmd);
            strcat(destPath,pathinfo->stack.path[pathinfo->stack.top -1]);
            strcat(destPath,"/");
            strcat(pathinfo->curPath,"/");
            strcat(pathinfo->curPath,pathinfo->stack.path[pathinfo->stack.top -1]);
            }
        cmd = strtok(NULL," /");
    }
    printf("destPath = %s\n",destPath);
    
    // 切换到用户命令目录
    if (stat(destPath, &st) == -1) {
        sprintf(response, "Path does not exist: %s", destPath);
        responseCode = PATH_ERROR;
        goto END;
    }
    if (chdir(destPath) == -1) {
        sprintf(response, "Failed to change directory: %s", destPath);
        responseCode = PATH_ERROR;
        goto END;
    }
    printf("Curpath = %s\n",pathinfo->curPath);
    sprintf(response, "Successfully changed to directory: %s", pathinfo->curPath);
    
END:
    int ret = sendResponseCode(netfd, responseCode);
    if (ret == -1) {
        perror("send responseCode failed");
        return -1;
    }
    ret = sendTrain(netfd, response, strlen(response));
    if (ret == -1) {
        perror("send response failed");
        return -1;
    }
    
    return responseCode;
}

int lsCommand(PathInfo *pathinfo, int netfd) {
    char cmdCopy[1024] = {0};
    char lsPath[MAX_PATH_LEN] = {0};
    char lsResult[4096] = {0};
    DIR *dir = NULL;
    struct dirent *entry = NULL;
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
        strcpy(lsPath, "./");
    } else {
        while (cmd != NULL) {
            if (strlen(lsPath) + strlen(cmd) + 2 >= MAX_PATH_LEN) {
                return -1; 
            }
            strcat(lsPath, cmd);
            strcat(lsPath, "/");
            cmd = strtok(NULL, " /");
        }
    }
    dir = opendir(lsPath);
    if (!dir) {
        snprintf(lsResult, sizeof(lsResult), "Error: Cannot open directory %s\n", lsPath);
        write(netfd, lsResult, strlen(lsResult));
        return -1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strlen(lsResult) + strlen(entry->d_name) + 2 >= sizeof(lsResult)) {
            break;
        }
        strcat(lsResult, entry->d_name);
        strcat(lsResult, " ");
    }
    if (strlen(lsResult) > 0) {
        sendTrain(netfd, lsResult, strlen(lsResult));
    } else {
        const char *empty_msg = "Directory is empty\n";
        sendTrain(netfd, empty_msg, strlen(empty_msg));
    }
    closedir(dir);
    return 0;
}

int pwdCommand(PathInfo *pathinfo, int netfd){
    char path[MAX_PATH_LEN] = {0};
    bzero(path,sizeof(path));
    if(pathinfo->stack.top == 1){
        printf("path is empty\n");
        const char *empty_msg = "Have in root!\n";
        sendTrain(netfd, empty_msg, strlen(empty_msg));
        return 0;
    }
    for(int i = 1; i < pathinfo->stack.top; i++){
        printf("stack = %s\n",pathinfo->stack.path[i]);
        strcat(path,"/");
        strcat(path,pathinfo->stack.path[i]);
    }
    printf("path = %s\n",path);
    sendTrain(netfd,path,strlen(path));
    return 0;
}

int getCommand(PathInfo *pathinfo, int netfd){
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
    transFile(netfd,path);
}

int recvFile(int netfd) {
    char filename[4096] = {0};
    train_t train;
    int responseCode = SUCCESS;
    // Receive file name
    recvn(netfd, &train.length, sizeof(train.length));
    recvn(netfd, train.data, train.length);
    memcpy(filename, train.data, train.length);
    
    struct stat st;
    // Check if file already exists or is a directory
    if (stat(filename, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            printf("File %s already exists as a regular file.\n", filename);
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
            return -1;  // If it's a regular file, return error
        } else if (S_ISDIR(st.st_mode)) {
            printf("File %s is a directory.\n", filename);
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
            return -1;  // If it's a directory, return error
        }
    } else {
        if (errno == ENOENT) {
            printf("File %s does not exist. Proceeding to receive it.\n", filename);
            sendResponseCode(netfd,responseCode);
        } else {
            perror("stat error");
            return -1;  // If stat fails, return error
        }
    }

    // Receive file size
    off_t filesize;
    recvn(netfd, &train.length, sizeof(train.length));
    recvn(netfd, train.data, train.length);
    memcpy(&filesize, train.data, train.length);

    printf("Receiving file: %s\n", filename);
    printf("File size: %.2f MB\n", (double)filesize / (1024 * 1024));

    // Create and map file
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
    ERROR_CHECK(fd, -1, "open");

    int ret = ftruncate(fd, filesize);
    ERROR_CHECK(ret, -1, "ftruncate");

    // Memory map the file
    char *p = (char*)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(p, MAP_FAILED, "mmap");

    // Receive file data into the mapped memory
    recvn(netfd, p, filesize);

    // Clean up resources
    munmap(p, filesize);
    close(fd);

    printf("File transfer completed successfully!\n");
    return 0;
}

int putCommand(int netfd){
    int ret = recvFile(netfd);
    int responseCode = SUCCESS;
    if(ret == -1){
        responseCode = PATH_ERROR;
    }
    int sret = sendResponseCode(netfd, responseCode);
    if (sret == -1) {
        perror("send responseCode failed");
    }
}

int rmCommand(PathInfo *pathinfo, int netfd){
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
    int responseCode = SUCCESS;
    struct stat st;
    // Check if file already exists or is a directory
    if (stat(path, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            remove(path);
            responseCode = SUCCESS;
            sendResponseCode(netfd,responseCode);
              // If it's a regular file, return error
        } else if (S_ISDIR(st.st_mode)) {
            if (rmdir(path) == 0) {
                responseCode = SUCCESS;
                sendResponseCode(netfd,responseCode);
                printf("Directory '%s' deleted successfully.\n", path);
            } else {
                responseCode = PATH_ERROR;
                sendResponseCode(netfd,responseCode);
                perror("Error deleting directory");
                return -1;
            }
        }
    } else {
        if (errno == ENOENT) {
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
        } else {
            perror("stat error");
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
            return -1;  // If stat fails, return error
        }
    }
    return 0;
}
int mkCommand(PathInfo *pathinfo, int netfd){
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
    int responseCode = SUCCESS;
    struct stat st;
    // Check if file already exists or is a directory
    if (stat(path, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
              // If it's a regular file, return error
        } else if (S_ISDIR(st.st_mode)) {
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
        }
    } else {
        if (errno == ENOENT) {
            mkdir(path, 0777);
            responseCode = SUCCESS;
            sendResponseCode(netfd,responseCode);
        } else {
            perror("stat error");
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
            return -1;  // If stat fails, return error
        }
    }
    return 0;
}
int clientCommand(int netfd) {
    train_t train;
    PathInfo pathinfo;
    ssize_t ret;
    // 依次接收路径信息
    memset(&pathinfo, 0, sizeof(PathInfo));
    stackInit(&pathinfo.stack);
    for (int i = 0; i < 2; ++i) {
        recvTrain(netfd,&train);
        // 将路径存储到对应字段
        if (i == 0){
            memcpy(pathinfo.curPath, train.data, train.length);
            printf("currpath = %s\n",pathinfo.curPath);}
        else if (i == 1)
            memcpy(pathinfo.rootPath, train.data, train.length);
    }
    //先接受栈的top
    // int top = 0;
    // //recvInt(netfd,&top);
    // printf("top = %d\n",top);

    //接受stack.path
    char buffer[1024];
    ssize_t rret = recv(netfd,buffer,sizeof(buffer),0);
    char **data = NULL;
    size_t count = 0;
    deserialize(buffer, rret, &data, &count);
    size_t copy_count = count > MAX_STACK_LEN ? MAX_STACK_LEN : count;
    memcpy(pathinfo.stack.path,data,copy_count * sizeof(char *));
    pathinfo.stack.top = copy_count;
    for(size_t i = 0; i < count; i++){
        printf("String %zu: %s\n",i,data[i]);
        printf("path = %s\n",pathinfo.stack.path[i]);
        printf("top = %d\n",pathinfo.stack.top);
    }
    free(data);
    //END

    // recvTrain(netfd,&train);
    // deserializePathStack(&pathinfo.stack,buffer,BUFSIZE);
    printf("top = %d\n",pathinfo.stack.top);
    printf("stack = %s\n",pathinfo.stack.path[0]);

    static int isFirstExecution = 1; 
    if(isFirstExecution){
        checkCommand(&pathinfo,netfd);
        isFirstExecution = 0;
    }
    printf("1\n");
    recvTrain(netfd,&train);
    memcpy(pathinfo.commands.args, train.data, train.length);
    commandAnalyze(&pathinfo);
    printf("curPath = %s, rootPath = %s, args = %s\n", pathinfo.curPath, pathinfo.rootPath, pathinfo.commands.args);
    printf("pathinfo.commands.type = %d\n",pathinfo.commands.type);
    printf("Start to recv clientCommand\n");
    switch (pathinfo.commands.type) {
        case cd:
            printf("switch to cd\n");
            cdCommand(&pathinfo,netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
        case ls:
            printf("switch to ls\n");
            lsCommand(&pathinfo,netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
        case pwd:
            printf("switch to pwd\n");
            pwdCommand(&pathinfo,netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
        case put:
            printf("switch to put\n");
            putCommand(netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
        case get:
            printf("switch to get\n");
            getCommand(&pathinfo,netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
        case rm:
            printf("switch to rm\n");
            rmCommand(&pathinfo,netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
        case mk:
            printf("switch to mk\n");
            mkCommand(&pathinfo,netfd);
            printf("done!\n");
            printf("-------------------------------------------\n");
            break;
            break;
    }
    return 0;
}