#include "../include/head.h"
#include "client.h"

int stackInit(PathStack *stack){
    stack->capacity = MAX_STACK_LEN;
    stack->top = 0;
    memset(stack->path, 0, sizeof(char*) * MAX_STACK_LEN);
    stack->path[stack->top++] =strdup(ROOTPATH);
}
int stackPush(PathStack *stack, const char *path){
    if (stack->top >= stack->capacity - 1){
        return -1;
    }
    stack->path[stack->top++] =strdup(path);
    return 0;
}
int stackPop(PathStack *stack){
    if(stack->top == 0){
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
    // printf("cmd = %s\n", cleanCmd);  // 调试输出
    if (strcmp(cleanCmd, "cd") == 0) return cd;
    if (strcmp(cleanCmd, "ls") == 0) return ls;
    if (strcmp(cleanCmd, "pwd") == 0) return pwd;
    if (strcmp(cleanCmd, "put") == 0) return put;
    if (strcmp(cleanCmd, "get") == 0) return get;
    if (strcmp(cleanCmd, "rm") == 0) return rm;
    if (strcmp(cleanCmd, "mk") == 0) return mk;
    return -1;
}
int sendTrain(int sockfd, const char *data, int length) {
    train_t train;
    train.length = length;
    memcpy(train.data, data, length);
    return send(sockfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL);
}
int commandAnalyze(PathInfo *pathinfo){
    // 去掉末尾的换行符
        char cmdCopy[1024] = {0};
        size_t len = strlen(pathinfo->commands.args);
        if (len > 0 && pathinfo->commands.args[len-1] == '\n') {
            pathinfo->commands.args[len-1] = '\0';
        }
        // 复制命令以供解析
        bzero(cmdCopy, sizeof(cmdCopy));
        strcpy(cmdCopy, pathinfo->commands.args);
        // 先用空格分割命令和参数
        char *cmd = strtok(cmdCopy, " /");
        if (cmd == NULL) {
            return 0;
        }
        // 获取命令类型
        pathinfo->commands.type = getCommandType(cmd);
        // 获取参数（如果有的话）
        char *args = strtok(NULL, "\n");
        printf("Command: '%s'\n", cmd);
        printf("Arguments: '%s'\n", args ? args : "");
        printf("Command type: %d\n", pathinfo->commands.type);
}

int recvResponseCode(int netfd, int *responseCode) {
    int netResponseCode;
    ssize_t ret = recv(netfd, &netResponseCode, sizeof(netResponseCode), 0);
    if (ret != sizeof(netResponseCode)) {
        fprintf(stderr, "Failed to receive responseCode\n");
        *responseCode = PATH_ERROR;
        return -1;
    }
    *responseCode = ntohl(netResponseCode); // 转换为主机字节序
    return 0;
}
int sendResponseCode(int netfd, int responseCode) {
    int netResponseCode = htonl(responseCode); // 转换为网络字节序
    ssize_t ret = send(netfd, &netResponseCode, sizeof(netResponseCode), 0);
    if (ret != sizeof(netResponseCode)) {
        fprintf(stderr, "Failed to send responseCode\n");
        return -1;
    }
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

