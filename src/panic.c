#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // IWYU pragma: keep // Weird? warning on clangd
#include <time.h>
#include <unistd.h>

#include "panic.h"
#include "logger/logger.h"
#include "stacktrace/stacktrace.h"
#include "util/util.h"

static const char* snip1 = "--------[ stacktrace UwU ]--------";
static const char* snip2 = "----------------------------------";

static atomic_bool hasPanic;
static atomic_bool hasHardPanic;

static pthread_once_t panicControl = PTHREAD_ONCE_INIT;
static pthread_once_t hardPanicControl = PTHREAD_ONCE_INIT;

DEFINE_LOGGER_STATIC(logger, "Panic Handler");
#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&logger)

static void panicOccured() {
  atomic_init(&hasPanic, false);
}

static void hardPanicOccured() {
  atomic_init(&hasHardPanic, false);
}

static void dumpStacktrace(void (^printMsg)(const char* fmt, ...)) {
  printMsg("%s", snip1);
  
  int res = stacktrace_walk_through_stack(^int (struct stacktrace_element* element) {
    if (element->sourceInfoPresent)
      printMsg("  at %s(%s:%d)", element->printableName, element->sourceFile, element->sourceLine);
    else if (element->symbolName)
      printMsg("  at %s(Source.c:-1)", element->symbolName);
    else
      printMsg("  at %p(Source.c:-1)", (void*) element->ip);
    
    if (element->count > 1)
      printMsg("  ... previous frame repeats %d times ...", element->count - 1);
    return 0;
  });
  
  if (res == -ENOSYS)
    printMsg("Stacktrace unavailable");
  
  printMsg("%s", snip2);
}

void _hard_panic_va(const char* panicFmt, va_list list) {
  pthread_once(&hardPanicControl, hardPanicOccured);
  
  // Double panic, sleep forever
  while (atomic_exchange(&hasHardPanic, true) == true)
    sleep(1000);
  
  // Using static buffer as panic might occur on OOM scenario
  static char panicBuffer[512 * 1024] = {0};
  
  size_t panicMsgLen = vsnprintf(panicBuffer, sizeof(panicBuffer), panicFmt, list);
  fprintf(stderr, "Program HARD panic (log not flushed): %s\n", panicBuffer);
  if (panicMsgLen >= sizeof(panicBuffer)) 
    fprintf(stderr, "Panic message was truncated! (%zu bytes to %zu bytes)\n", panicMsgLen, sizeof(panicBuffer) - 1); 
  
  dumpStacktrace(^(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputs("\n", stderr);
    va_end(args);
  });
  abort();
}

void _hard_panic(const char* fmt, ...) {
  pthread_once(&hardPanicControl, hardPanicOccured);
  
  va_list args;
  va_start(args, fmt);
  _hard_panic_va(fmt, args);
  va_end(args);
}

void _panic_va(const char* panicFmt, va_list panicArgs) {
  pthread_once(&panicControl, panicOccured);
  
  // Double panic, sleep forever
  while (atomic_exchange(&hasHardPanic, true) == true)
    sleep(1000);
  
  printk_va(LOG_FATAL, panicFmt, panicArgs);
  dumpStacktrace(^(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printk_va(LOG_FATAL, fmt, args);
    va_end(args);
  });
  
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
