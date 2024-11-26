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

int recvTrain(int sockfd, train_t *train){
    ssize_t ret;
    // 接收长度字段
    ret = recv(sockfd, &train->length, sizeof(train->length), 0);
    if (ret <= 0 || ret != sizeof(train->length)) {
        fprintf(stderr, "Failed to receive length\n");
        return -1;
    }
    // 检查数据长度是否合理
    if (train->length > BUFSIZE) {
        fprintf(stderr, "Path length exceeds size\n");
        return -1;
    }
    // 接收数据字段
    ret = recv(sockfd, train->data, train->length, 0);
    if (ret <= 0 || ret != train->length) {
        fprintf(stderr, "Failed to receive data\n");
        return -1;
    }
    return 0;  // 成功
}

void updateProgress(off_t current, off_t total) {
    float percentage = (float)current / total * 100;
    int filled = (int)((float)current / total * PROGRESS_WIDTH);
    
    printf("\r");
    
    printf("[");
    for (int i = 0; i < PROGRESS_WIDTH; i++) {
        if (i < filled) {
            printf("=");
        } else {
            printf(" ");
        }
    }
    printf("] %.1f%%", percentage);
    
    if (current == total) {
        printf("\n");
    }
    
    fflush(stdout); 
}

int recvn(int sockfd, void *buf, long total) {
    char *p = (char *)buf;
    long cursize = 0;
    while (cursize < total) {
        ssize_t sret = recv(sockfd, p + cursize, total - cursize, 0);
        if (sret == 0) {
            return 1;
        }
        cursize += sret;
    }
    return 0;
}

int recvWithProgress(int sockfd, char *buf, long total) {
    char *p = (char *)buf;
    long cursize = 0;
    while (cursize < total) {
        ssize_t sret = recv(sockfd, p + cursize, total - cursize, 0);
        if (sret == 0) {
            return 1;
        }
        cursize += sret;
        updateProgress(cursize, total);
    }
    return 0;
}

int recvFile(int sockfd) {

    char filename[4096] = {0};
    int responseCode = SUCCESS;
    train_t train;
    char *buffer = NULL;
    recvResponseCode(sockfd,&responseCode);
    if(responseCode == PATH_ERROR){
        printf("The file does not exist!\n");\
        return 0;
    }
    
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,&train.data,train.length);
    memcpy(filename,train.data,train.length);
    int fd = open(filename,O_RDWR);
    if(fd == -1){
        responseCode = PATH_NOT_EXIST;
        sendResponseCode(sockfd,responseCode);
        off_t filesize;
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        memcpy(&filesize,train.data,train.length);
        printf("Receiving file: %s\n", filename);
        printf("File size: %.2f MB\n", (double)filesize / (1024 * 1024));
        fd = open(filename,O_CREAT|O_RDWR|O_TRUNC,0666);
        buffer = malloc(filesize);
        recvWithProgress(sockfd,buffer,filesize);
        if(write(fd, buffer, filesize) != filesize){
            printf("Write file error!\n");
            free(buffer);
            close(fd);
            return -1;
        }
        free(buffer);
        close(fd);
    }else{
        responseCode = PATH_EXIST;
        sendResponseCode(sockfd,responseCode);
        struct stat st;
        train.length = sizeof(off_t);
        memcpy(train.data,&st.st_size,train.length);
        send(sockfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL);
        recvResponseCode(sockfd,&responseCode);
        if(responseCode == PATH_EXIST){
            printf("The file already exists!\n");
            close(fd);
            return 0;
        }else if(responseCode = PATH_NOT_EXIST){
            printf("The file is incomplete. Prepare to accept the remaining file contents\n");
            off_t filesize;
            recvn(sockfd,&train.length,sizeof(train.length));
            recvn(sockfd,train.data,train.length);
            memcpy(&filesize,train.data,train.length);
            printf("Receiving file: %s\n", filename);
            printf("File size: %.2f MB\n", (double)filesize / (1024 * 1024));
            lseek(fd,0,SEEK_END);
            buffer = malloc(filesize);
            recvWithProgress(sockfd,buffer,filesize);
            if(write(fd, buffer, filesize) != filesize){
                printf("Write file error!\n");
                free(buffer);
                close(fd);
                return -1;
            }
            free(buffer);
            close(fd);
        }
    }
    responseCode = SUCCESS;
    sendResponseCode(sockfd,responseCode);
    printf("Successfully!\n");
    return 0;
}

void serialize(char *data[], size_t count, char **buffer, size_t *total_size){
    *total_size = 0;

    for(size_t i = 0; i < count; i++){
        *total_size += strlen(data[i]) + 1; 
    }
    *buffer = malloc(*total_size);
    if(*buffer == NULL){
        perror("Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    char *ptr = *buffer;
    for (size_t i = 0; i < count; i++){
        size_t len = strlen(data[i]) + 1;
        memcpy(ptr, data[i], len);
        ptr += len;
    }
}

int transFile(int netfd, const char *path) {
    int responseCode = SUCCESS;
    train_t train;
    char md5_str[MD5_LEN] = {0};
    int cfm_ret = Compute_file_md5(path,md5_str);
    if(cfm_ret == 0){
        printf("[file - %s] md5 value:\n", path);
		printf("%s\n", md5_str);
    }
    train.length = strlen(md5_str);
    memcpy(train.data,md5_str,train.length);
    send(netfd,&train,sizeof(train.length)+train.length,0);
    
    recvResponseCode(netfd,responseCode);
    if(responseCode == PATH_EXIST){
        train.length = strlen(path);
        memcpy(train.data,path,train.length);
        send(netfd,&train,sizeof(train.length)+train.length,0);
        recvResponseCode(netfd,&responseCode);
        if(responseCode == PATH_ERROR){
            printf("File already exist\n");
            return 0;
        }else{
            printf("Upload file successful!\n");
        }
    }else{
        train.length = strlen(path);
        memcpy(train.data,path,train.length);
        send(netfd,&train,sizeof(train.length)+train.length,0);

    }

    char filePath[4096] = {0};
    
    strncpy(filePath, path, sizeof(filePath) - 1);
    
    struct stat statbuf;
    int fd = open(filePath, O_RDWR);
    if (fd == -1) {
        perror("File open error");
        return -1;
    }
    // 获取文件信息
    if (fstat(fd, &statbuf) == -1) {
        perror("fstat error");
        close(fd);
        return -1;
    }
    // 获取文件名
    char *filename = basename(filePath);    
    // 发送文件名
    train.length = strlen(filename);
    printf("filename %s\n",filename);
    memcpy(train.data, filename, train.length);
    if (send(netfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL) == -1) {
        perror("send filename failed");
        close(fd);
        return -1;
    }
    // 接收响应码
    if (recvResponseCode(netfd, &responseCode) == -1) {
        perror("recv response code failed");
        close(fd);
        return -1;
    }
    // 定义大文件阈值 100M
    const off_t LARGE_FILE_THRESHOLD = 100 * 1024 * 1024;  // 100MB
    if (responseCode == PATH_EXIST) {
        // 接收已传输的文件大小
        off_t st_size = 0;
        if(recv(netfd,&train.length,sizeof(train.length),0) == -1){
            close(fd);
            return -1;
        }
        if(recv(netfd,&train.data,train.length,0) == -1){
            close(fd);
            return -1;
        }
        
        memcpy(&st_size, train.data, sizeof(off_t));
        // 检查是否需要续传
        if (st_size == statbuf.st_size) {
            responseCode = PATH_ERROR;
            sendResponseCode(netfd,responseCode);
            printf("Upload lose! Path is exist!\n");
            close(fd);
            return -1;
        }
        // 从断点处继续传输
        if (lseek(fd, st_size, SEEK_SET) == -1) {
            perror("lseek failed");
            close(fd);
            return -1;
        }
        memset(&train,0,sizeof(train));
        // 发送剩余文件大小
        train.length = sizeof(off_t);
        off_t s_size = statbuf.st_size - st_size;
        printf("%ld\n",s_size);
        char buffer[sizeof(off_t)];
        memcpy(train.data, &s_size, sizeof(off_t));
        printf("trainlenth = %s\n",train.data);
        if (send(netfd,&train,sizeof(train.length)+train.length,MSG_NOSIGNAL) == -1) {
            perror("send file size failed");
            close(fd);
            return -1;
        }
        
        // 大文件使用 mmap
        if (statbuf.st_size > LARGE_FILE_THRESHOLD) {
            void *mapped = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (mapped == MAP_FAILED) {
                perror("mmap failed");
                close(fd);
                return -1;
            }
            
            // 发送剩余文件内容
            ssize_t sent = send(netfd, (char*)mapped + st_size, statbuf.st_size - st_size, MSG_NOSIGNAL);
            if (sent == -1) {
                perror("send mmap content failed");
                munmap(mapped, statbuf.st_size);
                close(fd);
                return -1;
            }
            // 取消内存映射
            munmap(mapped, statbuf.st_size);
        } else {
            // 小文件使用 sendfile
            if (sendfile(netfd, fd, NULL, statbuf.st_size - st_size) == -1) {
                perror("sendfile failed");
                close(fd);
                return -1;
            }
        }
    } else if(responseCode == PATH_ERROR){
        printf("It's a directory Can't trans\n");
        return -1;
    }else if(responseCode == PATH_NOT_EXIST){
        // 发送总文件大小
        train.length = sizeof(off_t);
        memcpy(train.data, &statbuf.st_size, train.length);
        if (send(netfd, &train, sizeof(train.length) + train.length, MSG_NOSIGNAL) == -1) {
            perror("send file size failed");
            close(fd);
            return -1;
        }
        // 大文件使用 mmap
        if (statbuf.st_size > LARGE_FILE_THRESHOLD) {
            void *mapped = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (mapped == MAP_FAILED) {
                perror("mmap failed");
                close(fd);
                return -1;
            }
            // 发送整个文件内容
            ssize_t sent = send(netfd, mapped, statbuf.st_size, MSG_NOSIGNAL);
            if (sent == -1) {
                perror("send mmap content failed");
                munmap(mapped, statbuf.st_size);
                close(fd);
                return -1;
            }
            // 取消内存映射
            munmap(mapped, statbuf.st_size);
        } else {
            // 小文件使用 sendfile
            if (sendfile(netfd, fd, NULL, statbuf.st_size) == -1) {
                perror("sendfile failed");
                close(fd);
                return -1;
            }
        }
    }
    close(fd);
    printf("File transfer Done!\n");
    return 0;
}

/*file md5*/
int Compute_file_md5(const char *file_path, char *md5_str)
{
	int i;
	int fd;
	int ret;
	unsigned char data[READ_DATA_SIZE];
	unsigned char md5_value[MD5_SIZE];
	MD5_CTX md5;

	fd = open(file_path, O_RDONLY);
	if (-1 == fd)
	{
		perror("open");
		return -1;
	}

	// init md5
	MD5Init(&md5);

	while (1)
	{
		ret = read(fd, data, READ_DATA_SIZE);
		if (-1 == ret)
		{
			perror("read");
			close(fd);
			return -1;
		}

		MD5Update(&md5, data, ret);

		if (0 == ret || ret < READ_DATA_SIZE)
		{
			break;
		}
	}

	close(fd);

	MD5Final(&md5, md5_value);

	// convert md5 value to md5 string
	for(i = 0; i < MD5_SIZE; i++)
	{
		snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
	}

	return 0;
}

int put_command(int netfd,char *args){
    char path[256] = {0};
    char cmdCopy[256] = {0};
    if (strlen(args) >= sizeof(cmdCopy)) {
        return -1;
    }
    strcpy(cmdCopy, args);
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
    if(transFile(netfd,path) == -1){
        return -1;
    }
    return 0;
}

