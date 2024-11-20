#include "client.h"

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
    
    PathInfo pathinfo;
    PathInfoInit(&pathinfo);
    char cmdCopy[1024] = {0};

    while(1) {
        //recvFile(sockfd);
        // 读取命令
        fileCommand(&pathinfo,sockfd);
    }

    //
    close(sockfd);
    
    return 0;
}