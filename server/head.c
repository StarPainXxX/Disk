#include "../include/head.h"
#include "worker.h"
int recvn(int sockfd, void *buf, long total) {
    char *p = (char *)buf;
    long cursize = 0;
    while (cursize < total) {
        ssize_t sret = recv(sockfd, p + cursize, total - cursize, 0);
        if (sret == 0) {
            return 1;
        }else if(sret == -1){
            return -1;
        }
        cursize += sret;
    }
    return 0;
}
int stackInit(PathStack *stack){
    stack->top = -1;
    memset(stack->path, 0, sizeof(char*) * MAX_STACK_LEN);
}
int stackPush(PathStack *stack, const char *path){
    if (stack->top >= MAX_STACK_LEN - 1){
        return -1;
    }
    stack->path[stack->top + 1] =strdup(path);
    stack->top++;
    return 0;
}
int stackPop(PathStack *stack){
    if(stack->top == -1){
        printf("stack is empty\n");
        return -1;
    }
    stack->path[stack->top] = NULL;
    stack->top--;
}
CommandType getCommandType(const char *cmd){
    char cleanCmd[32] = {0};
    strncpy(cleanCmd, cmd, strlen(cmd));
    char *newline = strchr(cleanCmd, '\n');
    if (newline) *newline = '\0';
    // printf("cmd = %s\n", cleanCmd);  
    if (strcmp(cleanCmd, "cd") == 0) return cd;
    if (strcmp(cleanCmd, "ls") == 0) return ls;
    if (strcmp(cleanCmd, "pwd") == 0) return pwd;
    if (strcmp(cleanCmd, "put") == 0) return put;
    if (strcmp(cleanCmd, "get") == 0) return get;
    if (strcmp(cleanCmd, "rm") == 0) return rm;
    if (strcmp(cleanCmd, "mk") == 0) return mk;
    if (strcmp(cleanCmd, "quit") == 0) return quit;
    return -1;
}
int sendTrain(int sockfd, const char *data, int length) {
    train_t train;
    train.length = length;
    memcpy(train.data, data, length);
    return send(sockfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
}
int commandAnalyze(Command *commands){
    // 去掉末尾的换行符
        char cmdCopy[1024] = {0};
        size_t len = strlen(commands->args);
        if (len > 0 && commands->args[len-1] == '\n') {
            commands->args[len-1] = '\0';
        }
        // 复制命令以供解析
        bzero(cmdCopy, sizeof(cmdCopy));
        strcpy(cmdCopy, commands->args);
        // 先用空格分割命令和参数
        char *cmd = strtok(cmdCopy, " /");
        if (cmd == NULL) {
            return 0;
        }
        // 获取命令类型
        commands->type = getCommandType(cmd);
        // 获取参数（如果有的话）
        char *args = strtok(NULL, "\n");
        MY_LOG_INFO("Command: '%s'", cmd);
        MY_LOG_INFO("Arguments: '%s'", args ? args : "");
        MY_LOG_INFO("Command type: %d", commands->type);
}

int sendResponseCode(int netfd, int responseCode) {
    int netResponseCode = htonl(responseCode); // 转换为网络字节序
    ssize_t ret = send(netfd, &netResponseCode, sizeof(netResponseCode), 0);
    if (ret != sizeof(netResponseCode)) {
        MY_LOG_ERROR("Failed to send responseCode\n");
        return 1;
    }else if(ret == -1){
        return -1;
    }
    return 0;
}
int recvResponseCode(int netfd, int *responseCode) {
    int netResponseCode;
    ssize_t ret = recv(netfd, &netResponseCode, sizeof(netResponseCode), 0);
    if (ret != sizeof(netResponseCode)) {
        MY_LOG_ERROR("Failed to receive responseCode\n");
        return -1;
    }
    *responseCode = ntohl(netResponseCode); // 转换为主机字节序
    return 0;
}
int sendInt(int netfd, int value){
    char buffer[16]; 
    sprintf(buffer, "%d", value);
    send(netfd, buffer, strlen(buffer), 0);
    return 0;
}

int recvInt(int sock, int *value) {
    char buffer[16];
    recv(sock, buffer, sizeof(buffer), 0);
    *value = atoi(buffer); // 转回整数
    return 0;
}

int recvTrain(int sockfd, train_t *train){
    ssize_t ret;
    // 接收长度字段
    ret = recv(sockfd, &train->length, sizeof(train->length), 0);
    if (ret <= 0 || ret != sizeof(train->length)) {
        MY_LOG_ERROR("Failed to receive length\n");
        return -1;
    }
    // 检查数据长度是否合理
    if (train->length > BUFSIZE) {
        MY_LOG_ERROR("Received length exceeds max allowed size: %d",train->length);
        
    }
    // 接收数据字段
    ret = recv(sockfd, train->data, train->length, 0);
    if (ret <= 0 || ret != train->length) {
        MY_LOG_ERROR("Failed to receive data\n");
        return -1;
    }
    return 0;  // 成功
}