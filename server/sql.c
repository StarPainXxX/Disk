#include "sql.h"

int sql_connect(MYSQL **conn){
    char server[] = "cd-cdb-p69sqbze.sql.tencentcdb.com";
    char user[] = "root";
    char password[] = "032416Xx";
    char database[] = "MyDisk";
    int port = 27616;
    *conn = mysql_init(NULL);
    if(!mysql_real_connect(*conn,server,user,password,database,port,NULL,0)){
        MY_LOG_ERROR("Error connecting to database:%s\n",mysql_errno(*conn));
        exit(0);
    }else{
        MY_LOG_INFO("Database connected");
    }

    if (mysql_set_character_set(*conn, "utf8mb4") != 0) {
        MY_LOG_ERROR("Failed to set character set: %s\n", mysql_error(*conn));
        exit(0);
    }
    return 0;
}

int search_user(const char *name,char *salt,char *crypt_pw, MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query, MAX_SQL_LEN, "select id,salt,cryptpasswd,tomb from user where username = '%s'", name);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("Query failed: %s\n", mysql_errno(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if(res == NULL){
        MY_LOG_ERROR("Get result failed: %s\n", mysql_errno(mysql));
        return 1;
    }

    row = mysql_fetch_row(res);
    if(row == NULL){
        MY_LOG_ERROR("User");
        mysql_free_result(res);
        return 0;//用户不存在
    }

    if(atoi(row[3]) == 1){
        MY_LOG_ERROR("User is delete");
        mysql_free_result(res);
        return -2;//用户已经被删除
    }
    
    strcpy(salt,row[1]);
    strcpy(crypt_pw,row[2]);
    mysql_free_result(res);
    return atoi(row[0]);
}


int creat_user(const char *name, char *salt, char *crypt_pw, MYSQL *mysql) {
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    int user_id = -1;
    char pwd[256] = "/";
    strcat(pwd,name);
    mysql_set_character_set(mysql, "utf8mb4");
    
    for(int i = 0; i < strlen(name); i++) {
        printf("%02X ", (unsigned char)name[i]);
    }

    snprintf(query, MAX_SQL_LEN,
             "INSERT INTO user (username, salt, cryptpasswd, pwd, tomb) VALUES ('%s', '%s', '%s', '%s', %d);",
             name, salt, crypt_pw, pwd, 0);
    if (mysql_query(mysql, query) != 0) {
        MY_LOG_ERROR("Insert query failed: %s\n", mysql_error(mysql));
        return 1;
    }

    
    user_id = mysql_insert_id(mysql); 
    if (user_id == 0) {
        MY_LOG_ERROR("Failed to retrieve the last inserted ID.\n");
        return 1;
    }
    return user_id;
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
    return salt;
}

int out_user(const char *name,MYSQL *mysql){
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "UPDATE user set tomb = 1 where username = '%s';",
            name);
    if (mysql_query(mysql, query) != 0) {
        MY_LOG_ERROR("Update query failed: %s\n", mysql_error(mysql));
        return 1;
    }
    
    return 0;
}

int search_user_up(const char *name,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    MY_LOG_INFO("start to search user");
    snprintf(query, MAX_SQL_LEN, "select id from user where username = '%s'", name);
    
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("Query failed: %s\n", mysql_error(mysql));
        return 1;
    }
    MY_LOG_INFO("search over");
    res = mysql_store_result(mysql);
    if(res == NULL){
        MY_LOG_ERROR("Get result failed: %s\n", mysql_error(mysql));
        return 1;
    }

    row = mysql_fetch_row(res);
    if(row == NULL){
        mysql_free_result(res);
        return 0;
    }else{
        mysql_free_result(res);
        MY_LOG_ERROR("User is exists");
        return -1;
    }
}

//根据id获取rootpath
int pathInit(User *user,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN, "select pwd from user where id = '%d'",user->UserId);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("Query failed: %s\n", mysql_error(mysql));
        return 1;
    }
    res = mysql_store_result(mysql);
    if(res == NULL){
        MY_LOG_ERROR("Get result failed: %s\n", mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);
    strncpy(user->pathinfo.rootPath, row[0], MAX_PATH_LEN - 1);
    user->pathinfo.rootPath[MAX_PATH_LEN - 1] = '\0';  
    
    strncpy(user->pathinfo.curPath, user->pathinfo.rootPath, MAX_PATH_LEN - 1);
    user->pathinfo.curPath[MAX_PATH_LEN - 1] = '\0';

    return 0;
}

int check_dir(User *user, const char *path,int top,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,"select id from file where owner_id = %d and filename = '%s' and parent_id = %d;",
                                user->UserId,path,top+1);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }
    res = mysql_store_result(mysql);
    if(res == NULL){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);
    if(row == NULL){
        mysql_free_result(res);
        return 1;
    }else{
        mysql_free_result(res);
        return atoi(row[0]);
    }
    
}

int first_dir_init(User *user,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    int dir_id = -1;
    char path[256] = "/";
    strcat(path,user->UserName);
    snprintf(query,MAX_SQL_LEN,"insert into file (parent_id,filename,owner_id,type) values (%d,'%s',%d,%d);",
                                0,path,user->UserId,0);
    
    if(mysql_query(mysql,query) != 0){
        printf("%s\n",mysql_error(mysql));
        return 1;
    }

    dir_id = mysql_insert_id(mysql);
    if (dir_id == 0) {
        MY_LOG_ERROR("Failed to retrieve the last inserted ID.\n");
        return 1;
    }

    return dir_id;
}

int ls_dir(User *user,char *path,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select filename from file where owner_id = %d and parent_id = %d",
            user->UserId,user->pathinfo.stack.top + 1);

    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL){
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    int num_rows = mysql_num_rows(res);
    int is_first = 1;
    while((row = mysql_fetch_row(res))){
        if(!is_first){
            strcat(path," ");
        }
        strcat(path,row[0]);
        is_first = 0;
    }

    mysql_free_result(res);
    return num_rows;
}