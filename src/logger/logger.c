#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "logger.h"
#include "concurrency/mutex.h"
#include "panic.h"
#include "util/circular_buffer.h"

#ifdef BUG
#  undef BUG
#endif
#ifdef BUG_ON
#  undef BUG_ON
#endif

// BUG in here is hard panic
// because Fox cannot tell
// whether it caused in logger
// or not so just hard panic
#define BUG() hard_panic("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); 

#define BUG_ON(cond) do { \
  if (cond)  \
    BUG(); \
} while(0)

#define LOG_BUFFER_SIZE (8 * 1024 * 1024)
#define LOCAL_BUFFER_SIZE (96 * 1024)
static_assert(LOCAL_BUFFER_SIZE - 4096 > 0, "4 KiB extra must be available");

DEFINE_LOGGER_STATIC(defaultLogger, "Misc");
DEFINE_CIRCULAR_BUFFER_STATIC(buffer, LOG_BUFFER_SIZE);

static const char* loglevelToString(enum logger_loglevel level) {
  switch (level) {
    case LOG_FATAL:
      return "FATAL";
    case LOG_ALERT:
      return "ALERT";
    case LOG_CRITICAL:
      return "CRITICAL";
    case LOG_ERROR:
      return "ERROR";
    case LOG_WARN:
      return "WARN";
    case LOG_NOTICE:
      return "NOTICE";
    case LOG_INFO:
      return "INFO";
    case LOG_VERBOSE:
      return "VERBOSE";
    case LOG_DEBUG:
      return "DEBUG";
  }
  BUG();
}

void logger_doPrintk_va(struct logger* logger, enum logger_loglevel level, const char* location, const char* func, const char* fmt, va_list args) {
  static thread_local char outputBuffer[LOCAL_BUFFER_SIZE];
  static thread_local char messageBuffer[LOCAL_BUFFER_SIZE - 4096];
  
  if (!logger)
    logger = &defaultLogger;
  
  vsnprintf(messageBuffer, sizeof(messageBuffer), fmt, args);
  
  struct timespec currentTime;
  clock_gettime(CLOCK_REALTIME, &currentTime);
  
  struct tm brokenDownDateAndTime;
  if (!localtime_r(&currentTime.tv_sec, &brokenDownDateAndTime) && errno == -EOVERFLOW) {
    // Defaults to 1st January 1970, 00:00 AM
    brokenDownDateAndTime = (struct tm) {
      .tm_hour = 0,
      .tm_min = 0,
      .tm_sec = 0,
      .tm_isdst = false,
      .tm_mday = 1,
      .tm_mon = 1,
      .tm_year = 1970,
      .tm_wday = 4,
      .tm_yday = 1,
    };
  }
  
  static char timestampBuffer[1024];
  strftime(timestampBuffer, sizeof(timestampBuffer), "%a %d %b %Y, %H:%M:%S %z", &brokenDownDateAndTime);
  
  // Format is [timestamp] [subsystemName] [ThreadName/loglevel] [FileSource.c:line#function()] Message
  // Example: [Sat 12 Aug 2023, 10:31 AM +0700] [Renderer] [Render Thread/INFO] [renderer/renderer.c:20#init()] Initalizing OpenGL...
  size_t messageLen = snprintf(outputBuffer, sizeof(outputBuffer), "[%s] [%s] [%s/%s] [%s%s()] %s", timestampBuffer, logger->subsystemName, util_get_thread_name(), loglevelToString(level), location, func, messageBuffer);
  
  struct logger_entry header = {
    .len = messageLen,
    .logLevel = level,
    .time = util_timespec_to_double(&currentTime),
    .message = NULL
  };
  
  mutex_lock(&buffer.writeLock);
  mutex_lock(&buffer.lock);
  circular_buffer_write(&buffer, CIRCULAR_BUFFER_NO_LOCK, &header, sizeof(header), NULL);
  circular_buffer_write(&buffer, CIRCULAR_BUFFER_NO_LOCK, outputBuffer, header.len, NULL);
  mutex_unlock(&buffer.lock);
  mutex_unlock(&buffer.writeLock);
}

ATTRIBUTE_PRINTF(5, 6)
void logger_doPrintk(struct logger* logger, enum logger_loglevel level, const char* location, const char* func, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger_doPrintk_va(logger, level, location, func, fmt, args);
  va_end(args);
}

int logger_flush(struct timespec* abstimeout) {
  pr_info("Flushing logs...");
  pr_info("Flushing logs... (Duplicate is normal as to ensure previous one has flushed)");
  int ret = 0;
  if ((ret = circular_buffer_flush(&buffer, abstimeout ? CIRCULAR_BUFFER_WITH_TIMEOUT : 0, abstimeout)) < 0) {
    if (ret < 0)
      pr_error("Timed out flushing log");
    else
      pr_error("Unknown error during flushing log: %d", ret);
  }
  
  return ret;
}

void logger_get_entry(struct logger_entry* entry) {
  static thread_local char messageBuffer[LOCAL_BUFFER_SIZE + 1];
  
  mutex_lock(&buffer.readLock);
  
  mutex_lock(&buffer.lock);
  circular_buffer_read(&buffer, CIRCULAR_BUFFER_NO_LOCK, entry, sizeof(*entry), NULL);
  circular_buffer_read(&buffer, CIRCULAR_BUFFER_NO_LOCK, messageBuffer, entry->len, NULL);
  mutex_unlock(&buffer.lock);
  
  // NUL terminator was excluded
  messageBuffer[entry->len] = '\0';
  mutex_unlock(&buffer.readLock);
  
  entry->message = messageBuffer;
}

static void loggerCleanup() {
  struct timespec timeout;
  clock_gettime(CLOCK_REALTIME, &timeout);
  util_add_timespec(&timeout, LOGGER_DEFAULT_FLUSH_TIMEOUT);
  logger_flush(&timeout);
}

void logger_init() {
  atexit(loggerCleanup);
}
