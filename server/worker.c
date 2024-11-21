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
            if(clientCommand(netfd) == -1){
                break;
            }
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
    
        if (stat(pathinfo->rootPath, &st) == -1) {
            sprintf(response, "User does not exist: %s", pathinfo->rootPath);
            MY_LOG_ERROR("%s\n",response);
            responseCode = PATH_ERROR;
            goto END;
        }
        if (!S_ISDIR(st.st_mode)) {
            sprintf(response, "Not a User: %s", pathinfo->rootPath);
            MY_LOG_ERROR("%s\n",response);
            responseCode = PATH_ERROR;
            goto END;
        }
        // 3. 检查访问权限
        if (access(pathinfo->rootPath, X_OK) == -1) {
            sprintf(response, "Permission denied: %s", pathinfo->rootPath);
            MY_LOG_ERROR("%s\n",response);
            responseCode = PERMISSION_ERROR;
            goto END;
        }
        // 4. 切换目录
        if (chdir(pathinfo->rootPath) == -1) {
            sprintf(response, "Failed to change directory: %s", "-1");
            MY_LOG_ERROR("%s\n",response);
            responseCode = PATH_ERROR;
            goto END;
        }
        // 5. 获取当前工作目录（用于确认和返回）
        char currentPath[MAX_PATH_LEN] = {0};
        if (getcwd(currentPath, MAX_PATH_LEN) == NULL) {
            sprintf(response, "Failed to get current directory");
            MY_LOG_ERROR("%s\n",response);
            responseCode = PATH_ERROR;
            goto END;
        }
        // 将标志位设置为0，表示后续执行不再是第一次
        
    
    sprintf(response, "Successfully log in, %s", "Enjoy!");
    
END:
    
    int ret = sendResponseCode(netfd, responseCode);
    if (ret == -1) {
        MY_LOG_ERROR("send responseCode failed");
    }
    ret = sendTrain(netfd, response, strlen(response));
    if (ret == -1) {
        MY_LOG_ERROR("send response failed");
    }
}

int checkEnter(User user){
    MY_LOG_INFO("Star to checkEnter\n");
    user.UserName[strcspn(user.UserName,"\n")] = 0;
    if(strlen(user.UserName) == 0){
        MY_LOG_ERROR("Username is empty!\n");
        return -1;
    }
    printf("username = %s\n",user.UserName);
    struct spwd *pinfo = getspnam(user.UserName);

    if(pinfo == NULL){
       MY_LOG_ERROR("Get user fail!\n");
        return -1;
    }
    user.Passward[strcspn(user.Passward, "\n")] = 0;

    char salt[50];
    MY_LOG_INFO("entry = %s\n",pinfo->sp_pwdp);
    sscanf(pinfo->sp_pwdp, "$%*[^$]$%[^$]", salt);
    MY_LOG_INFO("盐值: %s\n", salt);

    char *encrypted_input = crypt(user.Passward, pinfo->sp_pwdp);
    
    if(encrypted_input == NULL){
        MY_LOG_ERROR("Encryption failed!\n");
        return -1;
    }
    if(strcmp(encrypted_input, pinfo->sp_pwdp) == 0){
        MY_LOG_INFO("Successful!\n");
        return 0;
    } else {
        MY_LOG_ERROR("Password incorrect!\n");
        return -2;
    }
};

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
                strcat(newPath,"/");
                strcat(newPath,pathinfo->stack.path[i]);
            }
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
    MY_LOG_INFO("destPath = %s\n",destPath);
    
    // 切换到用户命令目录
    if (stat(destPath, &st) == -1) {
        sprintf(response, "Path does not exist: %s", destPath);
        MY_LOG_ERROR("%s\n",response);
        responseCode = PATH_ERROR;
        goto END;
    }
    if (chdir(destPath) == -1) {
        sprintf(response, "Failed to change directory: %s", destPath);
        MY_LOG_ERROR("%s\n",response);
        responseCode = PATH_ERROR;
        goto END;
    }
    sprintf(response, "Successfully changed to directory: %s", pathinfo->curPath);
    MY_LOG_INFO("%s\n",response);
    
END:
    int ret = sendResponseCode(netfd, responseCode);
    if (ret == -1) {
        MY_LOG_ERROR("send responseCode failed");
        return -1;
    }
    ret = sendTrain(netfd, response, strlen(response));
    if (ret == -1) {
        MY_LOG_ERROR("send response failed");
        return -1;
    }
    
    return 0;
}

int lsCommand(PathInfo *pathinfo, int netfd) {
    char cmdCopy[1024] = {0};
    char lsPath[MAX_PATH_LEN] = {0};
    char lsResult[4096] = {0};
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    if (!pathinfo || !pathinfo->commands.args) {
        return 0;
    }
    if (strlen(pathinfo->commands.args) >= sizeof(cmdCopy)) {
        return 0;
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
        MY_LOG_ERROR("%s\n",lsResult);
        write(netfd, lsResult, strlen(lsResult));
        return 0;
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
        if(sendTrain(netfd, lsResult, strlen(lsResult)) == -1){
            return -1;
        };
    } else {
        const char *empty_msg = "Directory is empty\n";
        if(sendTrain(netfd, empty_msg, strlen(empty_msg)) == -1){
            return -1;
        };
        MY_LOG_INFO("%s\n",empty_msg);
    }
    MY_LOG_INFO("Successful! Command: ls\n");
    closedir(dir);
    return 0;
}

int pwdCommand(PathInfo *pathinfo, int netfd){
    char path[MAX_PATH_LEN] = {0};
    bzero(path,sizeof(path));
    if(pathinfo->stack.top == 1){
        MY_LOG_ERROR("path is empty\n");
        const char *empty_msg = "Have in root!\n";
        if(sendTrain(netfd, empty_msg, strlen(empty_msg)) == -1){
            return -1;
        };
        return 0;
    }
    for(int i = 1; i < pathinfo->stack.top; i++){
        strcat(path,"/");
        strcat(path,pathinfo->stack.path[i]);
    }
    if(sendTrain(netfd,path,strlen(path))== -1){
        return -1;
    };
    MY_LOG_INFO("Successful! Command: pwd\n");
    return 0;
}

int getCommand(PathInfo *pathinfo, int netfd){
    char path[4096] = {0};
    char cmdCopy[1024] = {0};
    if (!pathinfo || !pathinfo->commands.args) {
        return 0;
    }
    if (strlen(pathinfo->commands.args) >= sizeof(cmdCopy)) {
        return 0;
    }
    strcpy(cmdCopy, pathinfo->commands.args);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL, " /");
    if (!cmd) {
        strcpy(path, "./");
    } else {
        while (cmd != NULL) {
            if (strlen(path) + strlen(cmd) + 2 >= MAX_PATH_LEN) {
                return 0; 
            }
            strcat(path, cmd);
            cmd = strtok(NULL, " /");
            if(cmd != NULL){
            strcat(path, "/");
            }
        }
    }
    if(transFile(netfd,path) == -1){
        return -1;
    }
    MY_LOG_INFO("Successful! Command: get\n");
    return 0;
}

int recvFile(int netfd) {
    char filename[4096] = {0};
    train_t train;
    int responseCode = SUCCESS;
    // Receive file name
    if(recvn(netfd, &train.length, sizeof(train.length)) == -1){
        return -1;
    }
    if(recvn(netfd, train.data, train.length) == -1){
        return -1;
    }
    memcpy(filename, train.data, train.length);
    
    struct stat st;
    // Check if file already exists or is a directory
    if (stat(filename, &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            MY_LOG_ERROR("File %s already exists as a regular file.\n", filename);
            responseCode = PATH_ERROR;
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
            return 1;  // If it's a regular file, return error
        } else if (S_ISDIR(st.st_mode)) {
            MY_LOG_ERROR("File %s is a directory.\n", filename);
            responseCode = PATH_ERROR;
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
            return 1;  // If it's a directory, return error
        }
    } else {
        if (errno == ENOENT) {
            MY_LOG_ERROR("File %s does not exist. Proceeding to receive it.\n", filename);
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
        } else {
            perror("stat error");
            return 1;  // If stat fails, return error
        }
    }

    // Receive file size
    off_t filesize;
    if(recvn(netfd, &train.length, sizeof(train.length)) == -1){
        return -1;
    }
    if(recvn(netfd, train.data, train.length) == -1){
        return -1;
    }
    memcpy(&filesize, train.data, train.length);

    MY_LOG_INFO("Receiving file: %s\n", filename);
    MY_LOG_INFO("File size: %.2f MB\n", (double)filesize / (1024 * 1024));

    // Create and map file
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if(fd == -1){
        MY_LOG_ERROR("open error");
        return 0;
    }
    int ret = ftruncate(fd, filesize);
    if(fd == -1){
        MY_LOG_ERROR("ftruncate error");
        return 0;
    }
    

    // Memory map the file
    char *p = (char*)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    MY_LOG_ERROR("mmap error");

    // Receive file data into the mapped memory
    if(recvn(netfd, p, filesize) == -1){
        return -1;
    }

    // Clean up resources
    munmap(p, filesize);
    close(fd);

    MY_LOG_INFO("File transfer completed successfully!\n");
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
        MY_LOG_ERROR("send responseCode failed");
        return -1;
    }
    MY_LOG_INFO("Successful! Command: put\n");
    return 0;
}

int rmCommand(PathInfo *pathinfo, int netfd){
    char path[4096] = {0};
    char cmdCopy[1024] = {0};
    if (!pathinfo || !pathinfo->commands.args) {
        return 0;
    }
    if (strlen(pathinfo->commands.args) >= sizeof(cmdCopy)) {
        return 0;
    }
    strcpy(cmdCopy, pathinfo->commands.args);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL, " /");
    if (!cmd) {
        strcpy(path, "./");
    } else {
        while (cmd != NULL) {
            if (strlen(path) + strlen(cmd) + 2 >= MAX_PATH_LEN) {
                return 0; 
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
        return 0;
    }
    if (strlen(pathinfo->commands.args) >= sizeof(cmdCopy)) {
        return 0;
    }
    strcpy(cmdCopy, pathinfo->commands.args);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL, " /");
    if (!cmd) {
        strcpy(path, "./");
    } else {
        while (cmd != NULL) {
            if (strlen(path) + strlen(cmd) + 2 >= MAX_PATH_LEN) {
                return 0; 
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
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
              // If it's a regular file, return error
        } else if (S_ISDIR(st.st_mode)) {
            responseCode = PATH_ERROR;
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
        }
    } else {
        if (errno == ENOENT) {
            mkdir(path, 0777);
            responseCode = SUCCESS;
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
        } else {
            perror("stat error");
            responseCode = PATH_ERROR;
            if(sendResponseCode(netfd,responseCode) == -1){
                return -1;
            }
            return -1;  // If stat fails, return error
        }
    }
    return 0;
}// 引入日志头文件

int clientCommand(int netfd) {
    MY_LOG_INFO("Starting ClientCommand");
    train_t train;
    PathInfo pathinfo;
    ssize_t ret;
    static int userCheck = 1;
    static User user;
    while(userCheck){
        int responseCode = SUCCESS;
        bzero(&user,sizeof(User));
        
        if(recvTrain(netfd,&train) == -1){
            MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
            return -1;
        }
        memcpy(user.UserName,train.data,train.length);
        MY_LOG_DEBUG("Receiving username :%s",user.UserName);
        
        if(recvTrain(netfd,&train) == -1){
            MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
            return -1;
        }
        memcpy(user.Passward,train.data,train.length);
        MY_LOG_DEBUG("Receiving password :%s",user.Passward);

        int cret = checkEnter(user);
        if(cret == 0){
            userCheck = 0;
            MY_LOG_INFO("User authentication successful :%s",user.UserName);
            if(sendResponseCode(netfd,responseCode) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
        }else if(cret == -1){
            MY_LOG_WARN("User authentication failed: Invalid username :%s",user.UserName);
            responseCode = USER_ERROR;
            if(sendResponseCode(netfd,responseCode) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
        }else if(cret == -2){
            MY_LOG_WARN("User authentication failed: Invalid password :%s",user.Passward);
            responseCode = PASSWARD_ERROR;
            if(sendResponseCode(netfd,responseCode) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
        }
    }
    
    // 依次接收路径信息
    memset(&pathinfo, 0, sizeof(PathInfo));
    stackInit(&pathinfo.stack);
    
    for (int i = 0; i < 2; ++i) {
        if(recvTrain(netfd,&train) == -1){
            MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
            return -1;
        }
        // 将路径存储到对应字段
        if (i == 0){
            memcpy(pathinfo.curPath, train.data, train.length);
            MY_LOG_DEBUG("Current path received: %s", pathinfo.curPath);
        }
        else if (i == 1){
            memcpy(pathinfo.rootPath, train.data, train.length);
            MY_LOG_DEBUG("Root path received: %s", pathinfo.rootPath);
        }
    }

    char buffer[1024];
    ssize_t rret = recv(netfd,buffer,sizeof(buffer),0);
    char **data = NULL;
    size_t count = 0;
    deserialize(buffer, rret, &data, &count);
    size_t copy_count = count > MAX_STACK_LEN ? MAX_STACK_LEN : count;
    
    MY_LOG_INFO("Received %zu paths for stack", copy_count);
    
    memcpy(pathinfo.stack.path,data,copy_count * sizeof(char *));
    pathinfo.stack.top = copy_count;
    
    free(data);

    static int isFirstExecution = 1; 
    if(isFirstExecution){
        MY_LOG_INFO("First execution, performing initial check");
        checkCommand(&pathinfo,netfd);
        isFirstExecution = 0;
    }

    if(recvTrain(netfd,&train) == -1){
        MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
        return -1;
    }
    memcpy(pathinfo.commands.args, train.data, train.length);
    
    MY_LOG_DEBUG("Received command arguments: %s", pathinfo.commands.args);
    
    commandAnalyze(&pathinfo);
    
    MY_LOG_INFO("Command Type: %d", pathinfo.commands.type);
    
    switch (pathinfo.commands.type) {
        case cd:
            MY_LOG_INFO("Executing CD command");
            if(cdCommand(&pathinfo,netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        case ls:
            MY_LOG_INFO("Executing LS command");
            if(lsCommand(&pathinfo,netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        case pwd:
            MY_LOG_INFO("Executing PWD command");
            if(pwdCommand(&pathinfo,netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        case put:
            MY_LOG_INFO("Executing PUT command");
            if(putCommand(netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        case get:
            MY_LOG_INFO("Executing GET command");
            if(getCommand(&pathinfo,netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        case rm:
            MY_LOG_INFO("Executing RM command");
            if(rmCommand(&pathinfo,netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        case mk:
            MY_LOG_INFO("Executing MK command");
            if(mkCommand(&pathinfo,netfd) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user.UserName);
                return -1;
            }
            break;
        default:
            MY_LOG_ERROR("Unknown command type: %d", pathinfo.commands.type);
            break;
    }
      // 关闭日志系统
    return 0;
}