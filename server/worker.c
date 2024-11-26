#include "worker.h"

int tidArrInit(tidArr_t *ptidArr, int workerNum){
    ptidArr->arr = (pthread_t *)calloc(workerNum,sizeof(pthread_t));
    ptidArr->workerNum = workerNum;
    return 0;
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
            printf("One child going to exit");
            pthread_mutex_unlock(&pthreadPool->mutex);
            pthread_exit(NULL);
        }
        netfd = pthreadPool->taskQueue.pFront->netfd;
        deQueue(&pthreadPool->taskQueue);
        pthread_mutex_unlock(&pthreadPool->mutex);
        MYSQL *mysql;
        pthread_mutex_lock(&pthreadPool->mutex);
        sql_connect(&mysql);
        pthread_mutex_unlock(&pthreadPool->mutex);
        //transFile(netfd);
        User user;
        while(1){
            int uc_ret = userCommand(netfd,&user,mysql);
            if(uc_ret == 1){
                break;
            }else if(uc_ret == -1){
                MY_LOG_ERROR("client is disconnection");
                return NULL;
            }
        }
        pathInit(&user,mysql);
        stackInit(&user.pathinfo.stack);
        stackPush(&user.pathinfo.stack,user.pathinfo.rootPath);
        while(1){
            int cc_ret = clientCommand(netfd,&user,mysql);
            if(cc_ret == -1){
                MY_LOG_ERROR("client is disconnection\n");
                break;
            }
        }
        mysql_close(mysql);
        close(netfd);
    }
}

int userCommand(int netfd, User *user,MYSQL *mysql){
    int responseCode;
    if(recvResponseCode(netfd,&responseCode) == -1){
        MY_LOG_ERROR("recv code error");
        return -1;
    }
    switch (responseCode)
    {
    case USER_LOG_IN:
        MY_LOG_INFO("User log in");
        int li_ret = log_in_command(netfd,user,mysql);
        if(li_ret == 0){
            return 0;
        }else if(li_ret == -1){
            return -1;
        }
        break;
    case USER_SIGN_UP:
        MY_LOG_INFO("User sign up");
        int su_ret = sign_up_command(netfd,user,mysql);
        if(su_ret == 0){
            return 0;
        }else if(su_ret == -1){
            return -1;
        }
        break;
    case USER_SIGN_OUT:
        MY_LOG_INFO("User sign out");
        int so_ret = sign_out_command(netfd,user,mysql);
        if(so_ret == 0){
            return 0;
        }else if(so_ret == -1){
            return -1;
        }
        break;
    }
    return 1;
}
//服务器先传输salt值给客户端，客户端crypt加密后，传输密码密文给服务器，服务器进行匹配后，判断是否成功
int log_in_command(int netfd,User *user, MYSQL *mysql){
    train_t train;
    int Code = SUCCESS;
    char salt[SALT_LEN] = {0};
    char crypt_pw[256] = {0};
    char check_crypt[256] = {0};
    if(recvTrain(netfd,&train) == -1){
        MY_LOG_ERROR("recv username error");
        return -1;
    }
    size_t clean_length = 0;
    for(size_t i = 0; i < train.length && i < sizeof(user->UserName) - 1; i++) {
        if(train.data[i] >= 32 && train.data[i] <= 126) {
            user->UserName[clean_length++] = train.data[i];
        }
    }
    user->UserName[clean_length] = '\0';
    if(clean_length == 0) {
        Code = USER_ERROR;
        int sr_ret = sendResponseCode(netfd, Code);
        ERROR_CHECK(sr_ret, -1, "Send code error");
        MY_LOG_INFO("Invalid username");
        return 0;
    }
    int ret = search_user(user->UserName,salt,crypt_pw,mysql);
    if(ret == -2){
        Code = USER_ERROR_OUT;
        MY_LOG_ERROR("User is sign out %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 0;
    }
    if(ret == 0){
        Code = USER_ERROR;
        MY_LOG_ERROR("User not exist %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 0;
    }
    Code = SUCCESS;
    int sr_ret = sendResponseCode(netfd,Code);
    ERROR_CHECK(sr_ret,-1,"Send code error");
    user->UserId = ret;
    int st_ret = sendTrain(netfd,salt,strlen(salt));
    ERROR_CHECK(st_ret,-1,"Send salt error");
    int rt_ret = recvTrain(netfd,&train);
    ERROR_CHECK(rt_ret,-1,"recv data error");
    memcpy(check_crypt,train.data,train.length);
    if(strcmp(check_crypt,crypt_pw) == 0){
        Code = SUCCESS;
        MY_LOG_DEBUG("User successfully log in %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 1;
    }else{
        Code = PASSWARD_ERROR;
        MY_LOG_ERROR("Passward error %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 0;
    }
}
int sign_up_command(int netfd,User *user, MYSQL *mysql){
    train_t train;
    int Code = SUCCESS;
    char salt[SALT_LEN] = {0};
    char crypt_pw[256] = {0};
    int rt_ret = recvTrain(netfd,&train);
    ERROR_CHECK(rt_ret,-1,"Get username error");
    size_t clean_length = 0;
    for(size_t i = 0; i < train.length && i < sizeof(user->UserName) - 1; i++) {
        if(train.data[i] >= 32 && train.data[i] <= 126) {
            user->UserName[clean_length++] = train.data[i];
        }
    }
    user->UserName[clean_length] = '\0';
    if(clean_length == 0) {
        Code = USER_ERROR;
        int sr_ret = sendResponseCode(netfd, Code);
        ERROR_CHECK(sr_ret, -1, "Send code error");
        MY_LOG_INFO("Invalid username");
        return 0;
    }
    int ret = search_user_up(user->UserName,mysql);
    if(ret == -1 || ret == 1){
        Code = USER_ERROR;
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        MY_LOG_INFO("Create user fail");
        return 0;
    }else{
        Code = SUCCESS;
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        memcpy(salt,generate_salt(),SALT_LEN);
        int st_ret = sendTrain(netfd,salt,strlen(salt));
        ERROR_CHECK(st_ret,-1,"Send salt error");
        rt_ret = recvTrain(netfd,&train);
        ERROR_CHECK(rt_ret,-1,"Recv crypt error");
        memcpy(crypt_pw,train.data,train.length);
        user->UserId = creat_user(user->UserName,salt,crypt_pw,mysql);
        int ff_ret = first_dir_init(user,mysql);
        Code = SUCCESS;
        int sr_set = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_set,-1,"Send code error");
        MY_LOG_INFO("Create user success");
        return 0;
    }
    return 0;
}
int sign_out_command(int netfd,User *user, MYSQL *mysql){
    train_t train;
    int Code = SUCCESS;
    char salt[SALT_LEN] = {0};
    char crypt_pw[256] = {0};
    char check_crypt[256] = {0};
    if(recvTrain(netfd,&train) == -1){
        MY_LOG_ERROR("recv username error");
        return -1;
    }
    size_t clean_length = 0;
    for(size_t i = 0; i < train.length && i < sizeof(user->UserName) - 1; i++) {
        if(train.data[i] >= 32 && train.data[i] <= 126) {
            user->UserName[clean_length++] = train.data[i];
        }
    }
    user->UserName[clean_length] = '\0';
    if(clean_length == 0) {
        Code = USER_ERROR;
        int sr_ret = sendResponseCode(netfd, Code);
        ERROR_CHECK(sr_ret, -1, "Send code error");
        MY_LOG_INFO("Invalid username");
        return 0;
    }
    int ret = search_user(user->UserName,salt,crypt_pw,mysql);
    if(ret == -2){
        Code = USER_ERROR_OUT;
        MY_LOG_ERROR("User is sign out %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 0;
    }
    if(ret == 0){
        Code = USER_ERROR;
        MY_LOG_ERROR("User not exist %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 0;
    }
    Code = SUCCESS;
    int sr_ret = sendResponseCode(netfd,Code);
    ERROR_CHECK(sr_ret,-1,"Send code error");
    int st_ret = sendTrain(netfd,salt,strlen(salt));
    ERROR_CHECK(st_ret,-1,"Send salt error");
    int rt_ret = recvTrain(netfd,&train);
    ERROR_CHECK(rt_ret,-1,"recv data error");
    memcpy(check_crypt,train.data,train.length);
    if(strcmp(check_crypt,crypt_pw) == 0){
        Code = SUCCESS;
        MY_LOG_DEBUG("User sign out check over %s",user->UserName);
        int ou_ret = out_user(user->UserName,mysql);
        if(ou_ret == -1){
            Code == USER_ERROR;
            int sr_ret = sendResponseCode(netfd,Code);
            ERROR_CHECK(sr_ret,-1,"Send code error");
            return 0;
        }else{
            Code == SUCCESS;
            int sr_ret = sendResponseCode(netfd,Code);
            ERROR_CHECK(sr_ret,-1,"Send code error");
            return 0; 
        }
        return 1;
    }else{
        Code = PASSWARD_ERROR;
        MY_LOG_ERROR("Passward error %s",user->UserName);
        int sr_ret = sendResponseCode(netfd,Code);
        ERROR_CHECK(sr_ret,-1,"Send code error");
        return 0;
    }  
}

int cdCommand(int netfd, User *user, char *args, MYSQL *mysql) {
    
    int responseCode = SUCCESS;
    char cmdCopy[1024] = {0};
    strncpy(cmdCopy, args, sizeof(cmdCopy) - 1);
    char *newline = strchr(cmdCopy, '\n');
    if (newline) {
        *newline = '\0';
    }
    char *cmd = strtok(cmdCopy, " /");
    if (cmd == NULL || strcmp(cmd, "cd") != 0) {
        MY_LOG_ERROR("Invalid cd command format");
        return 1;
    }
    cmd = strtok(NULL, " /"); 
    if (cmd == NULL) {
        MY_LOG_ERROR("No directory specified");
        return 1;
    }
    while (*cmd == ' ') cmd++;
    
    PathStack stack;
    stackInit(&stack);
    for(int i = 0; i <= user->pathinfo.stack.top; i++) {
        stackPush(&stack, user->pathinfo.stack.path[i]);
        MY_LOG_DEBUG("stack top %d",user->pathinfo.stack.top);
    }
    
    while(cmd != NULL) {
        MY_LOG_DEBUG("Processing path component: '%s'", cmd);
        char path[256] = "/";
        strcat(path,cmd); 
        if(strcmp(cmd, "..") == 0) {
            if(stack.top > 0){
                stackPop(&stack);
            }else{
                MY_LOG_ERROR("Have in root");
                responseCode = PATH_ERROR;
                sendResponseCode(netfd, responseCode);
                return 1;
            }
            char newPath[MAX_PATH_LEN] = {0};
            for(int i = 0; i <= stack.top; i++) {
                strcat(newPath, stack.path[i]);
            }
            memcpy(user->pathinfo.curPath, newPath, sizeof(newPath));
        } else {
            char stack_path[256] = {0};
            for(int i = 0; i <= stack.top;i++){
                strcat(stack_path,stack.path[i]);
            }
            strcat(stack_path,path);
            MY_LOG_DEBUG("stack_path = %s",stack_path);
            int dir_id = get_dir_id(stack_path,user->UserId,mysql);
            if(dir_id == 1) {
                MY_LOG_ERROR("Path not exist: '%s'", cmd);
                responseCode = PATH_NOT_EXIST;
                sendResponseCode(netfd, responseCode);
                return 1;
            } else {
                stackPush(&stack, path);
                strcat(user->pathinfo.curPath, "/");
                strcat(user->pathinfo.curPath, cmd);
            }
        }
        cmd = strtok(NULL, " /");
    }
    while(user->pathinfo.stack.top > -1){
        stackPop(&user->pathinfo.stack);
    }
    MY_LOG_DEBUG("user top %d",user->pathinfo.stack.top);
    for(int i = 0; i <= stack.top; i++){
        stackPush(&user->pathinfo.stack,stack.path[i]);
    }
    MY_LOG_DEBUG("user top %d",user->pathinfo.stack.top);

    MY_LOG_INFO("New current path = '%s'", user->pathinfo.curPath);
    responseCode = SUCCESS;
    sendResponseCode(netfd, responseCode);
    MY_LOG_DEBUG("Cd successful");
    return 0;
}

int lsCommand(int netfd,User *user,MYSQL *mysql){
    train_t train;
    int responseCode = SUCCESS;
    char path[MAX_LS_LEN] = {0};
    char stack_path[256] = {0};
    MY_LOG_DEBUG("stack_path = %s",user->pathinfo.stack.path[0]);
    for(int i = 0; i <= user->pathinfo.stack.top; i++){
        strcat(stack_path,user->pathinfo.stack.path[i]);
        
    }
    int parent_id = get_dir_id(stack_path,user->UserId,mysql);
    int ls_ret = ls_dir(user,path,parent_id,mysql);
    MY_LOG_DEBUG("path = %s",path);
    if(ls_ret == 0){
        responseCode = PATH_ERROR;
        sendResponseCode(netfd,responseCode);
        return 0;
    }else{
        responseCode = SUCCESS;
        sendResponseCode(netfd,responseCode);
    }
    int st_ret = sendTrain(netfd,path,strlen(path));
    ERROR_CHECK(st_ret,-1,"send path error");
    MY_LOG_INFO("Ls successful");
    return 0;
}

int pwdCommand(int netfd,User *user){
    char path[256] = {0};
    
    if(user->pathinfo.stack.top == 0){
        strcat(path, "/");
    }else{
        for(int i = 1; i <= user->pathinfo.stack.top; i++){
            strcat(path,user->pathinfo.stack.path[i]);
        }
    }
    MY_LOG_INFO("path = %s",path);
    int st_ret = sendTrain(netfd,path,strlen(path));
    ERROR_CHECK(st_ret,-1,"send path error");
    MY_LOG_INFO("Pwd successful");
    return 0;
}

int mkCommand(int netfd,User *user,char *args,MYSQL *mysql){
    train_t train;
    int responseCode = SUCCESS;
    int top = 0;
    char cmdCopy[1024] = {0};
    strncpy(cmdCopy, args, sizeof(cmdCopy) - 1);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL, " /");
    while (*cmd == ' ') cmd++;
    PathStack stack;
    stackInit(&stack);
    for(int i = 0; i <= user->pathinfo.stack.top; i++){
        stackPush(&stack, user->pathinfo.stack.path[i]);
    }
    while(cmd != NULL) {
        char path[256] = "/";
        strcat(path,cmd); 
        MY_LOG_DEBUG("Processing path component: '%s'", cmd);
        
        if(strcmp(cmd, "..") == 0) {
            if(stack.top > 0){
                stackPop(&stack);
            }else{
                MY_LOG_ERROR("Path not exist");
                responseCode = PATH_NOT_EXIST;
                sendResponseCode(netfd, responseCode);
                return 1;
            }
        } else {
            
            stackPush(&stack,path);
        }
        cmd = strtok(NULL, " /");
    }

    if(stack.top == 0){
        return 0;
    }
    char stack_path[256] = {0};
    int parent_id;
    if(stack.top != 1){
        for(int i = 0; i < stack.top; i++){
            strcat(stack_path,stack.path[i]);
        }
        parent_id = get_dir_id(stack_path,user->UserId,mysql);
        MY_LOG_INFO("%d",parent_id);
        
        if(parent_id == 0){
            MY_LOG_ERROR("Path not exist: '%s'", stack_path);
            responseCode = PATH_NOT_EXIST;
            sendResponseCode(netfd, responseCode);
            return 1;
        }
    }else{
        parent_id = get_root_id(user,mysql);
        MY_LOG_INFO("%d",parent_id);
    }
    MY_LOG_INFO("%d",parent_id);
    char *lastSlash = strrchr(stack.path[stack.top],'/');
    char *name = lastSlash+1;
    MY_LOG_DEBUG("name = %s",stack.path[stack.top]);
    MY_LOG_DEBUG("name = %s",name);
    int cd_ret = create_dir(user->UserId,&stack,name,parent_id,mysql);
    if(cd_ret == 0){
        responseCode = PATH_EXIST;
        int sr_ret = sendResponseCode(netfd,responseCode);
        ERROR_CHECK(sr_ret,-1,"send code error");
    }
    responseCode = SUCCESS;
    int sr_ret = sendResponseCode(netfd,responseCode);
    ERROR_CHECK(sr_ret,-1,"send code error");
    MY_LOG_INFO("Mk successful");
}
int rmCommand(int netfd,User *user,char *args,MYSQL *mysql){
    train_t train;
    int responseCode = SUCCESS;
    int top = 0;
    char cmdCopy[1024] = {0};
    strncpy(cmdCopy, args, sizeof(cmdCopy) - 1);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL, " /");
    while (*cmd == ' ') cmd++;
    PathStack stack;
    stackInit(&stack);
    for(int i = 0; i <= user->pathinfo.stack.top; i++){
        stackPush(&stack, user->pathinfo.stack.path[i]);
    }
    while(cmd != NULL) {
        char path[256] = "/";
        strcat(path,cmd); 
        MY_LOG_DEBUG("Processing path component: '%s'", cmd);
        
        if(strcmp(cmd, "..") == 0) {
            if(stack.top > 0){
                stackPop(&stack);
            }else{
                MY_LOG_ERROR("Path not exist");
                responseCode = PATH_NOT_EXIST;
                sendResponseCode(netfd, responseCode);
                return 1;
            }
        } else {
            
            stackPush(&stack,path);
        }
        cmd = strtok(NULL, " /");
    }

    if(stack.top == 0){
        return 0;
    }
    char path[256] = {0};
    for(int i = 0; i <= stack.top; i++){
        strcat(path,stack.path[i]);
    }
    int rd_ret = rm_dir(path,user,mysql);
    if(rd_ret == -2){
        responseCode == PATH_ERROR;
        int sr_ret = sendResponseCode(netfd,responseCode);
        ERROR_CHECK(sr_ret,-1,"send code error");
        MY_LOG_ERROR("The floder is not empty");
        return 0;
    }
    responseCode == SUCCESS;
    int sr_ret = sendResponseCode(netfd,responseCode);
    ERROR_CHECK(sr_ret,-1,"send code error");
    MY_LOG_INFO("Rm successful");
}

int putCommand(int netfd,User *user,MYSQL *mysql){
    train_t train;
    int responseCode = SUCCESS;
    char md5_str[MD5_LEN] = {0};
    char filename[256] = {0};
    recvn(netfd,&train.length,sizeof(train.length));
    recvn(netfd,&train.data,train.length);
    memcpy(md5_str,train.data,train.length);

    int mf_ret = md5_find(md5_str,mysql);
    if(mf_ret == 1){
        responseCode = PATH_EXIST;
        int sr_ret = sendResponseCode(netfd,responseCode);
        ERROR_CHECK(sr_ret,-1,"send code error");
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,&train.data,train.length);
        memcpy(filename,train.data,train.length);
        off_t filesize;
        get_file_size(filesize,md5_str,mysql);
        int cf_ret = create_file(user->UserId,&user->pathinfo.stack,filename,filesize,md5_str,mysql);
        if(cf_ret == 0){
            responseCode = PATH_ERROR;
            sr_ret = sendResponseCode(netfd,responseCode);
            ERROR_CHECK(sr_ret,-1,"send code error");
        }
        responseCode = SUCCESS;
        sr_ret = sendResponseCode(netfd,responseCode);
        ERROR_CHECK(sr_ret,-1,"send code error");
        return 0;
    }else{
        responseCode = PATH_NOT_EXIST;
        int sr_ret = sendResponseCode(netfd,responseCode);
        ERROR_CHECK(sr_ret,-1,"send code error");
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,&train.data,train.length);
        memcpy(filename,train.data,train.length);
        off_t filesize;
        filesize = get_size(user->UserId,&user->pathinfo.stack,filename,mysql);
        if(filesize == 0){
            
        }

    }

}

int clientCommand(int netfd, User *user,MYSQL *mysql){
    train_t train;
    Command commands;
    int rt_ret = recvTrain(netfd,&train);
    ERROR_CHECK(rt_ret,-1,"Recv command error");
    if (train.length >= sizeof(commands.args)) {
        train.length = sizeof(commands.args) - 1;  // 留一个字节给 null 终止符
    }
    memcpy(commands.args, train.data, train.length);
    commands.args[train.length] = '\0';

    MY_LOG_DEBUG("Received command arguments: %s", commands.args);
    char *newline = strchr(commands.args, '\n');
    if (newline) {
        *newline = '\0';
    }
    commandAnalyze(&commands);

    switch(commands.type){
        case cd:
            MY_LOG_INFO("Executing CD command, User: %d",user->UserId);
            if( cdCommand(netfd,user,commands.args,mysql) == -1){
                MY_LOG_ERROR("Client %d disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case ls:
            MY_LOG_INFO("Executing LS command, User: %d",user->UserId);
            if( lsCommand(netfd,user,mysql) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case pwd:
            MY_LOG_INFO("Executing PWD command, User: %d",user->UserId);
            if( pwdCommand(netfd,user) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case put:
            MY_LOG_INFO("Executing PUT command, User: %d",user->UserId);
            if( mkCommand(netfd,user,commands.args,mysql) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case get:
            MY_LOG_INFO("Executing GET command, User: %d",user->UserId);
            if(-1){
                MY_LOG_ERROR("Client %s disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case rm:
            MY_LOG_INFO("Executing RM command, User: %d",user->UserId);
            if(rmCommand(netfd,user,commands.args,mysql) == -1){
                MY_LOG_ERROR("Client %s disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case mk:
            MY_LOG_INFO("Executing MK command, User: %d",user->UserId);
            if(mkCommand(netfd,user,commands.args,mysql) == -1){
                MY_LOG_ERROR("Client %d disconnection!\n",user->UserId);
                return -1;
            }
            break;
        case quit:
            return -1;
        default:
            MY_LOG_ERROR("Unknown command type: %d, User: %s",commands.type,user->UserId);
            break;
    }
    return 1;
     
}