#include "log.h"


static MyLogLevel g_max_log_level = MY_LOG_LEVEL_DEBUG;

void log_init(const char* ident, int facility, MyLogLevel max_log_level) {
    openlog(ident, LOG_PID | LOG_CONS, facility);
    g_max_log_level = max_log_level;
}

void log_write(MyLogLevel level, const char* file, 
               const char* function, int line, 
               const char* format, ...) {
    if (level < g_max_log_level) {
        return;
    }

    va_list args;
    char log_buffer[1024];
    char full_msg[2048];
    const char* color_code = "";

    // 设置颜色
    switch (level) {
        case MY_LOG_LEVEL_DEBUG: color_code = "\033[36m"; break; // 青色
        case MY_LOG_LEVEL_INFO:  color_code = "\033[32m"; break; // 绿色
        case MY_LOG_LEVEL_WARN:  color_code = "\033[33m"; break; // 黄色
        case MY_LOG_LEVEL_ERROR: color_code = "\033[31m"; break; // 红色
        case MY_LOG_LEVEL_FATAL: color_code = "\033[41m"; break; // 红色背景
        default: color_code = "\033[0m"; break;                // 重置颜色
    }

    // 格式化用户消息
    va_start(args, format);
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);

    // 构建完整日志消息，包含文件、函数、行号信息
    snprintf(full_msg, sizeof(full_msg), 
             "%s[%s:%d in %s] %s\033[0m", // 添加颜色前缀和重置
             color_code, file, line, function, log_buffer);

    // 根据不同日志级别记录
    switch(level) {
        case MY_LOG_LEVEL_DEBUG:
            syslog(LOG_DEBUG, "%s", full_msg);
            printf("%s\n", full_msg);
            break;
        case MY_LOG_LEVEL_INFO:
            syslog(LOG_INFO, "%s", full_msg);
            printf("%s\n", full_msg);
            break;
        case MY_LOG_LEVEL_WARN:
            syslog(LOG_WARNING, "%s", full_msg);
            printf("%s\n", full_msg);
            break;
        case MY_LOG_LEVEL_ERROR:
            syslog(LOG_ERR, "%s", full_msg);
            printf("%s\n", full_msg);
            break;
        case MY_LOG_LEVEL_FATAL:
            syslog(LOG_CRIT, "%s", full_msg);
            printf("%s\n", full_msg);
            break;
        default:
            syslog(LOG_INFO, "%s", full_msg);
            printf("%s\n", full_msg);
            break;
    }
}


void log_close(void) {
    closelog();
}