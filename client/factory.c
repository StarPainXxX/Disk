#include "factory.h"
int user_command(int sockfd, User *user){
    printf("1. Log in\n");
    printf("2. Sign up\n");
    printf("3. Sign out\n");
    printf("4. Quit\n");
    char command;
    printf("Enter your choice: ");
    command = getchar();
    getchar();
    int responseCode;
    switch (command)
    {
    case '1':
        responseCode = USER_LOG_IN;
        sendResponseCode(sockfd,responseCode);
        int li_ret = log_in_command(sockfd,user);
        if(li_ret == 0){
            system("clear");
            printf("Password error, please try again!\n");
            sleep(1);
            system("clear");
            return 0;
        }else if(li_ret == 1){
            system("clear");
            printf("Log in successfully, Enjoy!\n");
            return 1;
        }else if(li_ret == -1){
            system("clear");
            printf("User already sign out!\n");
            sleep(1);
            system("clear");
        }else if(li_ret == -1){
            system("clear");
            printf("User does not exists!\n");
            sleep(1);
            system("clear");
        }
        break;
    case '2':
        responseCode = USER_SIGN_UP;
        sendResponseCode(sockfd,responseCode);
        int su_ret = sign_up_command(sockfd,user);
        if(su_ret == 0){
            system("clear");
            return 0;
        }else if(su_ret == 1){
            system("clear");
            return 1;
        }
        break;
    case '3':
        responseCode = USER_SIGN_OUT;
        sendResponseCode(sockfd,responseCode);
        int uso_ret = sign_out_command(sockfd,user);
        if(uso_ret == 0){
            system("clear");
            return 0;
        }else if(uso_ret == 1){
            system("clear");
            return 1;
        }
        break;
    case '4':
        printf("bey!\n");
        return -1;
    default:
        system("clear");    
        printf("Invalid command. Please try again.\n");
        sleep(1);
        system("clear"); 
        break;
    }
}
int log_in_command(int sockfd, User *user){
    train_t train;
    int Code = SUCCESS;
    char salt[SALT_LEN] = {0};
    char crypt_pw[256] = {0};
    printf("Please enter user:\n");
    fgets(user->UserName,sizeof(user->UserName),stdin);
    sendTrain(sockfd,user->UserName,strlen(user->UserName));
    printf("Please enter password:\n");
    fgets(user->Passward,sizeof(user->Passward),stdin);
    recvResponseCode(sockfd,&Code);
    if(Code == USER_ERROR_OUT){
        return -1;
    }
    if(Code == USER_ERROR){
        return -2;
    }
    recvTrain(sockfd,&train);
    memcpy(salt,train.data,train.length);
    char* crypt_result = crypt(user->Passward,salt);
    strncpy(crypt_pw, crypt_result, sizeof(crypt_pw) - 1);
    sendTrain(sockfd,crypt_pw,strlen(crypt_pw));
    memset(user->Passward,0,sizeof(user->Passward));
    memset(crypt_pw,0,sizeof(crypt_pw));
    recvResponseCode(sockfd,&Code);
    if(Code == SUCCESS){
        return 1;
    }else if(Code == PASSWARD_ERROR){
        return 0;
    }

}
int sign_up_command(int sockfd, User *user){
    train_t train;
    int Code = SUCCESS;
    char salt[SALT_LEN] = {0};
    char crypt_pw[256] = {0};
    printf("Please enter user:\n");
    fgets(user->UserName,sizeof(user->UserName),stdin);
    printf("Please enter password:\n");
    fgets(user->Passward,sizeof(user->Passward),stdin);
    sendTrain(sockfd,user->UserName,strlen(user->UserName));
    recvResponseCode(sockfd,&Code);
    if(Code == USER_ERROR){
        printf("User already exists\n");
        return 0;
    }
    recvTrain(sockfd,&train);
    memcpy(salt,train.data,train.length);
    char* crypt_result = crypt(user->Passward,salt);
    strncpy(crypt_pw, crypt_result, sizeof(crypt_pw) - 1);
    sendTrain(sockfd,crypt_pw,strlen(crypt_pw));
    memset(user->Passward,0,sizeof(user->Passward));
    memset(crypt_pw,0,sizeof(crypt_pw));
    recvResponseCode(sockfd,&Code);

    if(Code == SUCCESS){
        printf("Registered successfully\n");
        return 0;
    }
}
int sign_out_command(int sockfd, User *user){
    train_t train;
    int Code = SUCCESS;
    char salt[SALT_LEN] = {0};
    char crypt_pw[256] = {0};
    printf("Please enter user:\n");
    fgets(user->UserName,sizeof(user->UserName),stdin);
    sendTrain(sockfd,user->UserName,strlen(user->UserName));
    
    
    recvResponseCode(sockfd,&Code);
    if(Code == USER_ERROR_OUT){
        printf("User already sign out\n");
        return 0;
    }
    if(Code == USER_ERROR){
        printf("User not exist\n");
        return 0;
    }
    printf("Please enter password:\n");
    fgets(user->Passward,sizeof(user->Passward),stdin);
    recvTrain(sockfd,&train);
    memcpy(salt,train.data,train.length);
    char* crypt_result = crypt(user->Passward,salt);
    strncpy(crypt_pw, crypt_result, sizeof(crypt_pw) - 1);
    sendTrain(sockfd,crypt_pw,strlen(crypt_pw));
    memset(user->Passward,0,sizeof(user->Passward));
    memset(crypt_pw,0,sizeof(crypt_pw));
    recvResponseCode(sockfd,&Code);
    
    if(Code == SUCCESS){
        printf("Sign out successful! user: %s",user->UserName);
        return 0;
    }else{
        printf("Password error or user error,sign out fail");
        return 0;
    }
}
