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

int check_dir(int userId, const char *path,int parent_id,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,"select id from file where owner_id = %d and filename = '%s' and parent_id = %d and type = %d;",
                                userId,path,parent_id,0);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return -2;
    }
    res = mysql_store_result(mysql);
    if(res == NULL){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return -2;
    }
    row = mysql_fetch_row(res);
    if(row == NULL){
        mysql_free_result(res);
        return -2;
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
    snprintf(query,MAX_SQL_LEN,"insert into file (parent_id,filename,owner_id,type,path) values (%d,'%s',%d,%d,'%s');",
                                0,path,user->UserId,0,path);
    
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

int get_root_id(User *user,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    char name[256] = "/";
    strcat(name,user->UserName);

    snprintf(query,MAX_SQL_LEN,
            "select id from file where parent_id = %d and path = '%s';",
            0,name);
    if(mysql_query(mysql,query) != 0){
        printf("%s\n",mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL){
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }

    row = mysql_fetch_row(res);
    if(row == NULL){
        MY_LOG_ERROR("Path not exists");
        mysql_free_result(res);
        return 1;
    }

    mysql_free_result(res);
    return atoi(row[0]);
}

int ls_dir(User *user,char *path,int parent_id,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    memset(path,0,sizeof(path));
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select filename from file where owner_id = %d and parent_id = %d and type = %d",
            user->UserId,parent_id,0);

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
    snprintf(query,MAX_SQL_LEN,
            "select path from file where owner_id = %d and parent_id = %d and type = %d",
            user->UserId,parent_id,1);
    
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL){
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    int nums_rows = mysql_num_rows(res);
    while((row = mysql_fetch_row(res))){
        strcat(path," ");
        char *lastSlash = strrchr(row[0],'/');
        strcat(path,lastSlash+1);
    }

    mysql_free_result(res);
    MY_LOG_DEBUG("%d",num_rows + nums_rows);
    return num_rows + nums_rows;
}

int create_dir(int userId,PathStack *stack,char *name,int parent_id,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    MY_LOG_DEBUG("name = %s",name);
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select count(*) from file where owner_id = %d and parent_id = %d and filename = '%s' and type = %d",
            userId,parent_id,name,0);
     if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);

    int num = atoi(row[0]);
    MY_LOG_INFO("%d",num);
    if(num > 0 ){
        mysql_free_result(res);
        MY_LOG_ERROR("Dir already exists");
        return 0;
    }
    char stack_path[256] = {0};
    for(int i = 0; i <= stack->top;i++){
        strcat(stack_path,stack->path[i]);
    }
    snprintf(query,MAX_SQL_LEN,
            "insert into file (parent_id,filename,owner_id,type,path) values (%d,'%s',%d,%d,'%s')",
            parent_id,name,userId,0,stack_path);
    
    if(mysql_query(mysql,query) != 0){
        printf("%s\n",mysql_error(mysql));
        return 1;
    }

    int dir_id = mysql_insert_id(mysql);
    if (dir_id == 0) {
        mysql_free_result(res);
        MY_LOG_ERROR("Failed to retrieve the last inserted ID.\n");
        return 1;
    }

    return dir_id;
}

int get_dir_id(const char *path,int userId,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select id from file where owner_id = %d and path = '%s';",
            userId,path);

    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return -2;
    }

    res = mysql_store_result(mysql);
    if(res == NULL){
        MY_LOG_ERROR("mysql_store_resul failed: %s",mysql_errno(mysql));
        return -2;
    }

    row = mysql_fetch_row(res);
    if(row == NULL){
        MY_LOG_ERROR("Path not exists");
        mysql_free_result(res);
        return -2;
    }

    mysql_free_result(res);
    return atoi(row[0]);
}

int rm_dir(char *path, User *user, MYSQL *mysql) {
    int id = get_dir_id(path, user->UserId, mysql);
    MY_LOG_DEBUG("id = %d", id);
    
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    char md5_str[MD5_LEN + 1] = {0};
    char query[MAX_SQL_LEN];
    snprintf(query, MAX_SQL_LEN, 
             "SELECT type, md5 FROM file WHERE id = %d", id);
    
    if (mysql_query(mysql, query) != 0) {
        MY_LOG_ERROR("Query error: %s", mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL) {
        MY_LOG_ERROR("Store result failed: %s", mysql_error(mysql));
        return 1;
    }

    row = mysql_fetch_row(res);
    if (row == NULL) {
        MY_LOG_ERROR("No row found");
        mysql_free_result(res);
        return 1;
    }

    int type = atoi(row[0]);
    strncpy(md5_str, row[1], MD5_LEN);
    mysql_free_result(res);

    MY_LOG_DEBUG("type = %d", type);

    if (type == 1) {
        snprintf(query, MAX_SQL_LEN, 
                 "SELECT COUNT(*) FROM file WHERE md5 = '%s'", md5_str);
        
        if (mysql_query(mysql, query) != 0) {
            MY_LOG_ERROR("Query error: %s", mysql_error(mysql));
            return 1;
        }

        res = mysql_store_result(mysql);
        if (res == NULL) {
            MY_LOG_ERROR("Store result failed: %s", mysql_error(mysql));
            return 1;
        }

        row = mysql_fetch_row(res);
        int num = atoi(row[0]);
        mysql_free_result(res);

        snprintf(query, MAX_SQL_LEN, "DELETE FROM file WHERE id = %d", id);
        if (mysql_query(mysql, query) != 0) {
            MY_LOG_ERROR("Delete error: %s", mysql_error(mysql));
            return 1;
        }
        if (num < 2) {
            if (unlink(md5_str) != 0) {
                MY_LOG_ERROR("Failed to delete file: %s", md5_str);
            }
        }

        return 0;
    } else { 
        int ld_ret = ls_dir(user, path, id, mysql);
        if (ld_ret != 0) {
            return -2;
        }

        snprintf(query, MAX_SQL_LEN, "DELETE FROM file WHERE id = %d", id);
        if (mysql_query(mysql, query) != 0) {
            MY_LOG_ERROR("Delete error: %s", mysql_error(mysql));
            return 1;
        }
        return 0;
    }
}

int md5_find(const char *md5_str,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    MY_LOG_INFO("md5 = %s",md5_str);
    snprintf(query,MAX_SQL_LEN,
            "select count(*) from file where md5 = '%s';",
            md5_str);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s",mysql_errno(mysql));
        return 1;
    }
    res = mysql_store_result(mysql);
    if (res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);

    int num = atoi(row[0]);
    MY_LOG_INFO("%d",num);
    if(num > 0 ){
        mysql_free_result(res);
        MY_LOG_INFO("The file exist,file express");
        return 1;
    }
    return num;
}

int get_file_size(off_t *size,const char *md5_str,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select filesize from file where md5 = '%s';",
            md5_str);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s",mysql_errno(mysql));
        return 1;
    }
    res = mysql_store_result(mysql);
    if (res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);

    *size = atol(row[0]);
    MY_LOG_DEBUG("%ld",size);
    return 0;
}

int create_file(int userId,PathStack *stack,char *name,off_t filesize,const char *md5_str,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char path[MAX_PATH_LEN] = {0};
    char stack_path[256] = {0};
    for(int i = 0; i <= stack->top;i++){
        strcat(stack_path,stack->path[i]);
    }
    int parent_id = get_dir_id(stack_path,userId,mysql);
    
    MY_LOG_DEBUG("name = %s",name);
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select count(*) from file where owner_id = %d and parent_id = %d and md5 = '%s' and type = %d",
            userId,parent_id,md5_str,1);
     if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);

    int num = atoi(row[0]);
    MY_LOG_INFO("%d",num);
    if(num > 0 ){
        mysql_free_result(res);
        MY_LOG_ERROR("file already exists");
        return 0;
    }

    strcat(path,stack_path);
    strcat(path,"/");
    strcat(path,name);
    MY_LOG_DEBUG("%s",path);

    MY_LOG_DEBUG("%d,'%s',%d,%d,'%s','%s',%ld",parent_id,name,userId,1,path,md5_str,filesize);
    snprintf(query,MAX_SQL_LEN,
            "insert into file (parent_id,filename,owner_id,type,path,md5,filesize) values (%d,'%s',%d,%d,'%s','%s',%ld)",
            parent_id,md5_str,userId,1,path,md5_str,filesize);
    if(mysql_query(mysql,query) != 0){
        printf("%s\n",mysql_error(mysql));
        return 1;
    }

    int file_id = mysql_insert_id(mysql);
    if (file_id == 0) {
        mysql_free_result(res);
        MY_LOG_ERROR("Failed to retrieve the last inserted ID.\n");
        return 1;
    }

    return file_id;

}

off_t get_size(int userId,PathStack *stack,char *name,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char path[MAX_PATH_LEN] = {0};
    char stack_path[256] = {0};
    off_t filesize;
    for(int i = 0; i <= stack->top;i++){
        strcat(stack_path,stack->path[i]);
    }
    strcat(path,stack_path);
    strcat(path,"/");
    strcat(path,name);

    MY_LOG_DEBUG("name = %s",name);
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select filesize from file where owner_id = %d and path = '%s' and type = %d",
            userId,path,1);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }

    res = mysql_store_result(mysql);
    if (res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);
    if(row == NULL){
        MY_LOG_INFO("The file not exist,ready to recv upload");
        mysql_free_result(res);
        filesize = 0;
        return filesize;
    }
    filesize = atol(row[0]);

    return filesize;
}

int update_file(int userId,const char *name,off_t filesize,const char *md5_str,PathStack stack,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char path[MAX_PATH_LEN] = {0};
    char stack_path[256] = {0};
    for(int i = 0; i <= stack.top;i++){
        strcat(stack_path,stack.path[i]);
    }
    int parent_id = get_dir_id(stack_path,userId,mysql);
    strcat(path,stack_path);
    strcat(path,"/");
    strcat(path,name);
    
    MY_LOG_DEBUG("name = %s",name);
    char query[MAX_SQL_LEN];
    snprintf(query,MAX_SQL_LEN,
            "select id from file where owner_id = %d and parent_id = %d and path = '%s' and type = %d",
            userId,parent_id,path,1);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }
    res = mysql_store_result(mysql);
    if (res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s\n",mysql_error(mysql));
        return 1;
    }
    row = mysql_fetch_row(res);

    int id = atoi(row[0]);

    snprintf(query,MAX_SQL_LEN,
            "update file set filesize = %ld,md5 = '%s' where id = %d",
            filesize,md5_str,id);
    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s\n",mysql_error(mysql));
        return 1;
    }
    mysql_free_result(res);
    return 0;
}

char* find_file(int userId,const char *path,MYSQL *mysql){
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[MAX_SQL_LEN] = {0};

    snprintf(query,MAX_SQL_LEN,
            "select md5 from file where path = '%s'",
            path);

    if(mysql_query(mysql,query) != 0){
        MY_LOG_ERROR("%s",mysql_error(mysql));
        return NULL;
    }
    res = mysql_store_result(mysql);
    if(res == NULL){
        mysql_free_result(res);
        MY_LOG_ERROR("mysql_store_result failed: %s",mysql_error(mysql));
        return NULL;
    }
    row = mysql_fetch_row(res);
    if(row == NULL){
        return NULL;
    }
    mysql_free_result(res);
    
    return row[0];
}
