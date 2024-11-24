#include <mysql/mysql.h>
#include "../include/head.h"

int sql_connect(MYSQL **conn){
    char server[] = "cd-cdb-p69sqbze.sql.tencentcdb.com";
    char user[] = "root";
    char password[] = "032416Xx";
    char database[] = "MyDisk";
    int port = 27616;
    *conn = mysql_init(NULL);
    if(!mysql_real_connect(*conn,server,user,password,database,port,NULL,0)){
        printf("Error connecting to database:%d\n",mysql_errno(*conn));
        exit(0);
    }else{
       printf("Database connected\n");
    }
    return 0;
}
int search_user(const char *name,char *salt, MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query, MAX_SQL_LEN, "select id,salt,tomb from user where username = '%s'", name);
    if(mysql_query(mysql,query) != 0){
        return -1;
    }

    res = mysql_store_result(mysql);
    if(res == NULL){
        return -1;
    }

    row = mysql_fetch_row(res);
    if(row == NULL){
        mysql_free_result(res);
        return 0;//用户不存在
    }

    if(atoi(row[2]) == 1){
        mysql_free_result(res);
        return -2;//用户已经被删除
    }

    strncpy(salt,row[1],30);
    mysql_free_result(res);
    return atoi(row[0]);
}

char *generate_salt() {
    static char salt[SALT_LEN];
    memset(salt, 0, SALT_LEN);
    
    salt[0] = '$';
    salt[1] = '6';
    salt[2] = '$';
    salt[19] = '$';
    
    srand(time(NULL));
    for(int i = 3; i < SALT_LEN - 1; i++) {
        int flag = rand() % 3;
        switch (flag) {
            case 0:
                salt[i] = rand() % 26 + 'a';
                break;
            case 1:
                salt[i] = rand() % 26 + 'A';
                break;
            case 2:
                salt[i] = rand() % 10 + '0';
                break;
        }
    }
    salt[20] = '\0';
    printf("%s\n", salt);
    return salt;
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

int check_dir(User *user, const char *path,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,"select id from file where owner_id = %d and filename = '%s' and parent_id = %d;",
                                user->UserId,path,user->pathinfo.stack.top+1);
    if(mysql_query(mysql,query) != 0){
        printf("%s\n",mysql_error(mysql));
        return 1;
    }
    res = mysql_store_result(mysql);
    if(res == NULL){
        printf("%s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);
    if(row == NULL){
        printf("Dir not exist\n");
        mysql_free_result(res);
        return 1;
    }else{
        mysql_free_result(res);
        return atoi(row[0]);
    }
    
}

int cdCommand(int netfd,User *user,char *args,MYSQL *mysql){
    printf("%s\n",args);
    int responseCode = SUCCESS;
    char cmdCopy[1024] = {0};
    bzero(cmdCopy, sizeof(cmdCopy));
    strcpy(cmdCopy, args);
    char *cmd = strtok(cmdCopy, " /");
    cmd = strtok(NULL," /");
    PathStack stack;
    stackInit(&stack);
    for(int i = 0; i <= user->pathinfo.stack.top; i++){
        stackPush(&stack,user->pathinfo.stack.path[i]);
    }
    printf("%s",cmd);
    while(cmd != NULL){
        if(strcmp(cmd,"..") == 0){
            stackPop(&stack);
            char newPath[MAX_PATH_LEN] = {0};
            for(int i = 0; i <= stack.top; i++){
                strcat(newPath,"/");
                strcat(newPath,stack.path[i]);
            }
            memcpy(user->pathinfo.curPath,newPath,sizeof(newPath));
        }else{
            if(check_dir(user,cmd,mysql) == 1){
                printf("Path not exist\n");
                responseCode = PATH_ERROR;
                return 1;
            }else{
                stackPush(&stack,cmd);
                strcat(user->pathinfo.curPath,"/");
                strcat(user->pathinfo.curPath,cmd);
            }
        }
        cmd = strtok(NULL," /");
    }
    for(int i = user->pathinfo.stack.top + 1; i <= stack.top; i++){
        stackPush(&user->pathinfo.stack,stack.path[i]);
    }
    printf("curPath = %s\n",user->pathinfo.curPath);
    responseCode = SUCCESS;
    printf("Cd successful\n");
    return 0;
}


int main(int argc, char *argv[]){
    MYSQL *conn;
    sql_connect(&conn);
    User user;
    user.UserId = 2;
    char path[] = "/joel";
    memcpy(user.pathinfo.curPath,path,sizeof(path));
    printf("%s\n",user.pathinfo.curPath);
    stackInit(&user.pathinfo.stack);
    stackPush(&user.pathinfo.stack,user.UserName);
    int netfd = 1;
    char args[] = "cd dir1";
    cdCommand(netfd,&user,args,conn);

    
}