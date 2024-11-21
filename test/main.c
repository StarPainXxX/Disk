#include "../include/head.h"
#define ROOTPATH "Disk/User1"


int stackInit(PathStack *stack){
    stack->capacity = MAX_STACK_LEN;
    stack->top = 0;
    memset(stack->path, 0, sizeof(char*) * MAX_STACK_LEN);
    stack->path[stack->top++] =strdup(ROOTPATH);
}

int PathInfoInit(PathInfo *pathinfo){
    memcpy(pathinfo->rootPath,ROOTPATH,sizeof(ROOTPATH));
    memcpy(pathinfo->curPath,pathinfo->rootPath,sizeof(pathinfo->rootPath));
    memset(&pathinfo->commands,0,sizeof(pathinfo->commands));

    stackInit(&pathinfo->stack);
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
    printf("cmd = %s\n", cleanCmd);  // 调试输出
    if (strcmp(cleanCmd, "cd") == 0) return cd;
    if (strcmp(cleanCmd, "ls") == 0) return ls;
    if (strcmp(cleanCmd, "pwd") == 0) return pwd;
    if (strcmp(cleanCmd, "put") == 0) return put;
    if (strcmp(cleanCmd, "get") == 0) return get;
    if (strcmp(cleanCmd, "rm") == 0) return rm;
    if (strcmp(cleanCmd, "mk") == 0) return mk;
    return -1;
}

int fileCommand(PathInfo pathinfo,int sockfd){
    switch (pathinfo.commands.type)
    {
    case cd:
        printf("switch to cd\n");
        break;
    case ls:
        printf("switch to ls\n");
        break;
    }
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

// int main(){
//     int sockfd;
//     PathInfo pathinfo;
//     PathInfoInit(&pathinfo);
//     printf("top = %d path = %s\n",pathinfo.stack.top,pathinfo.stack.path[pathinfo.stack.top]);
//     char cmdCopy[1024];
//      while(1) {
//         // 读取命令
//         fgets(pathinfo.commands.args, sizeof(pathinfo.commands.args), stdin);
//         commandAnalyze(&pathinfo);
//         switch (pathinfo.commands.type) {
//         case cd:
//             printf("switch to cd\n");
//             char cmdCopy[1024] = {0};
//             bzero(cmdCopy, sizeof(cmdCopy));
//             strcpy(cmdCopy, pathinfo.commands.args);
//             char *cmd = strtok(cmdCopy, " /");
//             printf("cmd = %s\n",cmd);
//             cmd = strtok(NULL," /");
//             printf("cmd = %s\n",cmd);
//             while(cmd != NULL){
//                 if(strcmp(cmd,"..") == 0){
//                     printf("1\n");
//                     stackPop(&pathinfo.stack);
//                     printf("top = %d,cmd = %s\n",pathinfo.stack.top,pathinfo.stack.path[pathinfo.stack.top-1]);
//                     char newPath[MAX_PATH_LEN] = {0};
//                     memcpy(newPath,pathinfo.stack.path[0],sizeof(pathinfo.stack.path[0]));
//                     printf("newPath = %s\n",newPath);
//                     for(int i = 1; i < pathinfo.stack.top; i++){
//                         strcat(newPath,"/");
//                         strcat(newPath,pathinfo.stack.path[i]);
//                         printf("newPath = %s\n",newPath);
//                     }
//                     memcpy(pathinfo.curPath,newPath,sizeof(newPath));
//                     printf("currpath = %s\n",newPath);
//                 }else{
//                     printf("2\n");
//                     stackPush(&pathinfo.stack,cmd);
//                     printf("top = %d,path = %s\n",pathinfo.stack.top,pathinfo.stack.path[pathinfo.stack.top-1]);
//                     printf("curpath = %s\n",pathinfo.curPath);
//                     strcat(pathinfo.curPath,"/");
//                     strcat(pathinfo.curPath,pathinfo.stack.path[pathinfo.stack.top -1]);
//                     printf("curpath = %s\n",pathinfo.curPath);
//                 }
//                 cmd = strtok(NULL," /");
//                 printf("cmd = %s\n",cmd);
//             }
//             char currentPath[MAX_PATH_LEN] = {0};
//             getcwd(currentPath, MAX_PATH_LEN);
//             printf("currentPath = %s \n",currentPath);
//             printf("curpathPath = %s \n",pathinfo.curPath);
//             if (chdir(pathinfo.curPath) == -1) {
//                 printf("path is not \n");
//             }
//             getcwd(currentPath, MAX_PATH_LEN);
//             printf("currentPath = %s \n",currentPath);
//             printf("go!\n");
//             if (chdir("..") == -1) {
//                 printf("path is not \n");
//             }
//             printf("rootPath = %s \n",pathinfo.rootPath);
//             getcwd(currentPath, MAX_PATH_LEN);
//             printf("currentPath = %s \n",currentPath);
//             printf("yes!\n");
//             break;
//         case ls:
//             break;
//         case pwd:
//             break;
//         case put:
//             break;
//         case get:
//             break;
//         case rm:
//             break;
//         case mk:
//             break;
//     }    
//     }
    
//     return 0;
// }
void list_directory(const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path); // 打开指定路径的目录

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    printf("Contents of directory: %s\n", path);
    while ((entry = readdir(dir)) != NULL) { // 逐个读取目录内容
        printf("%s\n", entry->d_name); // 输出文件名或目录名
    }

    closedir(dir); // 关闭目录流
}
#include<crypt.h>
#include<shadow.h>
#define MAX_USER_NAME 256
#define MAX_PASSWARD 256


typedef struct{
    char UserName[MAX_USER_NAME];
    char Passward[MAX_PASSWARD];
}User;

int checkEnter(User user){
    user.UserName[strcspn(user.UserName,"\n")] = 0;
    if(strlen(user.UserName) == 0){
        printf("Username is empty!\n");
        return -1;
    }
    printf("username = %s\n",user.UserName);
    struct spwd *pinfo = getspnam(user.UserName);

    if(pinfo == NULL){
        printf("Get user fail!\n");
        return -1;
    }
    user.Passward[strcspn(user.Passward, "\n")] = 0;

    char salt[50];
    printf("entry = %s\n",pinfo->sp_pwdp);
    sscanf(pinfo->sp_pwdp, "$%*[^$]$%[^$]", salt);
    printf("盐值: %s\n", salt);

    char *encrypted_input = crypt(user.Passward, pinfo->sp_pwdp);
    
    if(encrypted_input == NULL){
        printf("Encryption failed!\n");
        return -1;
    }
    if(strcmp(encrypted_input, pinfo->sp_pwdp) == 0){
        printf("Successful!\n");
        return 0;
    } else {
        printf("Password incorrect!\n");
        return -1;
    }
};
int main() {
    User user;
    printf("请输入账号:\n");
    fgets(user.UserName,sizeof(user.UserName),stdin);
    printf("请输入密码:\n");
    fgets(user.Passward,sizeof(user.Passward),stdin);
    checkEnter(user);
    return 0;
}