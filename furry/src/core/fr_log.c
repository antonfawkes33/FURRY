#include "fr_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>


static const char *level_strings[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

#ifdef _WIN32
#include <windows.h>
static const char *level_colors[] = {"\x1b[90m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#else
static const char *level_colors[] = {"\x1b[90m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

void fr_log(FrLogLevel level, const char *file, int line, const char *fmt,
            ...) {
  time_t t = time(NULL);
  struct tm lt;
#ifdef _WIN32
  localtime_s(&lt, &t);
#else
  localtime_r(&t, &lt);
#endif

  char buf[16];
  strftime(buf, sizeof(buf), "%H:%M:%S", &lt);

  printf("%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf, level_colors[level],
         level_strings[level], file, line);

  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
  fflush(stdout);

  if (level == FR_LOG_FATAL) {
#ifdef _WIN32
    DebugBreak();
#endif
    exit(1);
  }
}
