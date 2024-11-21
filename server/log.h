#ifndef __SYSLOG__
#define __SYSLOG__

#include "../include/head.h"
typedef enum {
    MY_LOG_LEVEL_DEBUG = 0,
    MY_LOG_LEVEL_INFO,
    MY_LOG_LEVEL_WARN,
    MY_LOG_LEVEL_ERROR,
    MY_LOG_LEVEL_FATAL
} MyLogLevel;

/**
 * 日志系统初始化函数
 * @param ident 程序标识名
 * @param facility syslog设施
 * @param max_log_level 最大日志级别
 */
void log_init(const char* ident, int facility, MyLogLevel max_log_level);

/**
 * 日志记录函数
 * @param level 日志级别
 * @param file 源文件名
 * @param function 函数名
 * @param line 行号
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void log_write(MyLogLevel level, const char* file, 
               const char* function, int line, 
               const char* format, ...);

/**
 * 关闭日志系统
 */
void log_close(void);

// 日志宏定义，简化调用
#define MY_LOG_DEBUG(format, ...) \
    log_write(MY_LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define MY_LOG_INFO(format, ...) \
    log_write(MY_LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define MY_LOG_WARN(format, ...) \
    log_write(MY_LOG_LEVEL_WARN, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define MY_LOG_ERROR(format, ...) \
    log_write(MY_LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define MY_LOG_FATAL(format, ...) \
    log_write(MY_LOG_LEVEL_FATAL, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#endif


