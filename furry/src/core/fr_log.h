#ifndef FR_LOG_H
#define FR_LOG_H

typedef enum {
    FR_LOG_TRACE,
    FR_LOG_DEBUG,
    FR_LOG_INFO,
    FR_LOG_WARN,
    FR_LOG_ERROR,
    FR_LOG_FATAL
} FrLogLevel;

void fr_log(FrLogLevel level, const char *file, int line, const char *fmt, ...);

#define FR_TRACE(...) fr_log(FR_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define FR_DEBUG(...) fr_log(FR_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define FR_INFO(...)  fr_log(FR_LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define FR_WARN(...)  fr_log(FR_LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define FR_ERROR(...) fr_log(FR_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define FR_FATAL(...) fr_log(FR_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif // FR_LOG_H
