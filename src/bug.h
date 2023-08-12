#ifndef _headers_1667482340_CProjectTemplate_bug
#define _headers_1667482340_CProjectTemplate_bug

#include <stdlib.h>
#include <time.h>

#include "logger/logger.h"

// Linux kernel style bug 

// Time out needed incase of BUG inside
// logger thread which leads to deadlock
#define BUG() do { \
  pr_fatal("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
  struct timespec timeout = util_relative_to_abs(CLOCK_REALTIME, 5.0f); \
  logger_flush(&timeout); \
  abort(); \
} while(0)

#define BUG_ON(cond) do { \
  if (cond)  \
    BUG(); \
} while(0)

#define WARN() do { \
  pr_warn("WARN: warning at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
} while(0)

#define WARN_ON(cond) do { \
  if (cond)  \
    WARN(); \
} while(0)

#endif

