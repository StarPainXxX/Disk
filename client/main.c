#include "client.h"
#define ERROR_CHECK(ret,num,msg) {if(ret==num){printf(msg); return -1;}}
int main(int argc, char *argv[]) {
    ARGS_CHECK(argc, 3);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sockfd, -1, "socket");
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    
    int ret = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    ERROR_CHECK(ret, -1, "connect");
    
    User user;
    printf("-----------------------------\n");
    printf("Hi,there! Nice to meet you!\n");
    printf("Welcome to use!\n");
    printf("Please choose your option:\n");
    while(1){
        int uc_ret = user_command(sockfd,&user);
        if(uc_ret == 1){
            break;
        }else if(uc_ret == -1){
            close(sockfd);
            return 0;
        }
    }
    while(1) {
        int fc_ret = fileCommand(sockfd);
        if(fc_ret == -1){
            break;
            printf("bye!\n");
        }
    }

    //
    close(sockfd);
    
    return 0;
}