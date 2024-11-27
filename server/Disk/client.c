#include "client.h"

 // Width of the progress bar
// int PathInfoInit(PathInfo *pathinfo){
//     memcpy(pathinfo->rootPath,ROOTPATH,sizeof(ROOTPATH));
//     memcpy(pathinfo->curPath,pathinfo->rootPath,sizeof(pathinfo->rootPath));
//     memset(&pathinfo->commands,0,sizeof(pathinfo->commands));

//     stackInit(&pathinfo->stack);
// }

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
    case put:
        put_command(sockfd,commands.args);
        break;
    case -1:
        printf("Invalid command!\n");
        break;
    case quit:
        return -1;
    }
}




