#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "panic.h"
#include "logger/logger.h"

static atomic_bool hasPanic;
static atomic_bool hasHardPanic;

static pthread_once_t panicControl = PTHREAD_ONCE_INIT;
static pthread_once_t hardPanicControl = PTHREAD_ONCE_INIT;

static void panicOccured() {
  atomic_init(&hasPanic, false);
}

static void hardPanicOccured() {
  atomic_init(&hasHardPanic, false);
}

void _hard_panic_va(const char* fmt, va_list list) {
  pthread_once(&hardPanicControl, hardPanicOccured);
  
  // Double panic, sleep forever
  while (atomic_exchange(&hasHardPanic, true) == true)
    sleep(1000);
  
  // Using static buffer as panic might occur on OOM scenario
  static char panicBuffer[512 * 1024] = {0};
  
  size_t panicMsgLen = vsnprintf(panicBuffer, sizeof(panicBuffer), fmt, list);
  fprintf(stderr, "Program HARD panic: %s\n", panicBuffer);
  if (panicMsgLen >= sizeof(panicBuffer)) 
    fprintf(stderr, "Panic message was truncated! (%zu bytes to %zu bytes)\n", panicMsgLen, sizeof(panicBuffer) - 1); 
  abort();
}

void _hard_panic(const char* fmt, ...) {
  pthread_once(&hardPanicControl, hardPanicOccured);
  
  va_list args;
  va_start(args, fmt);
  _panic_va(fmt, args);
  va_end(args);
}

void _panic_va(const char* fmt, va_list args) {
  pthread_once(&panicControl, panicOccured);
  
  // Double panic, sleep forever
  while (atomic_exchange(&hasHardPanic, true) == true)
    sleep(1000);
  
  printk_va(LOG_FATAL, fmt, args);
  
  struct timespec timeout;
  clock_gettime(CLOCK_REALTIME, &timeout);
  util_add_timespec(&timeout, LOGGER_DEFAULT_FLUSH_TIMEOUT);
  logger_flush(&timeout);
  abort();
}

void _panic(const char* fmt, ...) {
  pthread_once(&panicControl, panicOccured);
  
  va_list args;
  va_start(args, fmt);
  _panic_va(fmt, args);
  va_end(args);
}
