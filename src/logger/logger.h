#ifndef _headers_1690279182_FluffyGC_logger
#define _headers_1690279182_FluffyGC_logger

#include <stddef.h>
#include <time.h>

#include "attributes.h"
#include "util/util.h"

enum logger_loglevel {
  LOG_FATAL,
  LOG_ALERT,
  LOG_CRITICAL,
  LOG_ERROR,
  LOG_WARN,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG
};

#define LOG_DEFAULT LOG_INFO
#define LOGGER_DEFAULT_FLUSH_TIMEOUT (5)

struct logger {
  const char* subsystemName;
};

struct logger_entry {
  enum logger_loglevel logLevel;
  float time;
  size_t len;
  const char* message;
};

#define LOGGER_INITIALIZER(name) { \
    .subsystemName = (name), \
  }
#define DEFINE_LOGGER(name, subsystemName)  \
  struct logger name = LOGGER_INITIALIZER(subsystemName);

// Format is [timestamp] [subsystemName] [ThreadName/loglevel] [FileSource.c:line#function()] Message
// Example: [Sat 12 Aug 2023, 10:31 AM +0700] [Renderer] [Render Thread/INFO] [renderer/renderer.c:20#init()] Initalizing OpenGL...

void logger_init();

// If logger is NULL, logs at default
ATTRIBUTE_PRINTF(5, 6)
void logger_doPrintk(struct logger* logger, enum logger_loglevel level, const char* location, const char* func, const char* fmt, ...);
void logger_doPrintk_va(struct logger* logger, enum logger_loglevel level, const char* location, const char* func, const char* fmt, va_list list);

// If abstimeout is NULL, wait indefinitely and never fails
// Error: 
// -ETIMEDOUT: Timed out waiting
// -EINVAL: Time out is invalid
int logger_flush(struct timespec* abstimeout);
void logger_get_entry(struct logger_entry* entry);

// Individual source should define this
// before including any files if printing
// to own logger
#ifndef LOGGER_DEFAULT
# define LOGGER_DEFAULT NULL
#endif

#define printk(level, fmt, ...) \
  logger_doPrintk(LOGGER_DEFAULT, (level), __FILE__ ":" stringify(__LINE__) "#", __func__, fmt __VA_OPT__(,) __VA_ARGS__)

#define printk_with_logger(logger, level, fmt, ...) \
  logger_doPrintk((logger), (level), __FILE__ ":" stringify(__LINE__) "#", __func__, (fmt) __VA_OPT__(,) __VA_ARGS__)

#define printk_va(level, fmt, va) \
  logger_doPrintk_va(LOGGER_DEFAULT, (level), __FILE__ ":" stringify(__LINE__) "#", __func__, (fmt), (va))

#define printk_with_logger_va(logger, level, fmt, va) \
  logger_doPrintk_va((logger), (level), __FILE__ ":" stringify(__LINE__) "#", __func__, (fmt), (va))

#define pr_debug(fmt, ...)    printk(LOG_DEBUG, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_info(fmt, ...)     printk(LOG_INFO, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_notice(fmt, ...)   printk(LOG_NOTICE, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_warn(fmt, ...)     printk(LOG_WARN, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_error(fmt, ...)    printk(LOG_ERROR, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_alert(fmt, ...)    printk(LOG_ALERT, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_crit(fmt, ...)     printk(LOG_CRITICAL, fmt __VA_OPT__(,) __VA_ARGS__)
#define pr_fatal(fmt, ...)    printk(LOG_FATAL, fmt __VA_OPT__(,) __VA_ARGS__)

#endif

