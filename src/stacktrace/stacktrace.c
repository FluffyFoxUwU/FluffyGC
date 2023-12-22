#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#include "macros.h"
#include "stacktrace/stacktrace.h"
#include "logger/logger.h"
#include "stacktrace/provider/libbacktrace.h"
#include "config.h"

static bool stacktraceEnabled = false;

static void cleanup() {
# if IS_ENABLED(CONFIG_STACKTRACE_PROVIDER_LIBBACKTRACE)
  stacktrace_libbacktrace_cleanup();
# endif
}

static void init() {
  int res = -ENOSYS;
# if IS_ENABLED(CONFIG_STACKTRACE_PROVIDER_LIBBACKTRACE)
  res = stacktrace_libbacktrace_init();
# endif
  
  if (res >= 0) {
    stacktraceEnabled = true;
    atexit(cleanup);
  } else {
    pr_error("Stacktrace will not be available due error %d during init", res);
  }
}

static pthread_once_t initControl = PTHREAD_ONCE_INIT;
static void tryInit() {
  pthread_once(&initControl, init);
}

void stacktrace_init() {
  tryInit();
}

int stacktrace_walk_through_stack(stacktrace_walker_block walker) {
  tryInit();
  
  int res = 0;
  
  __block struct stacktrace_element current = {};
  stacktrace_walker_block walkerBlock = ^int (struct stacktrace_element* element) {
    if (current.ip == element->ip) {
      current.count++;
      return 0;
    } else if (current.count >= 1) {
      int walkerRes = walker(&current);
      if (walkerRes < 0)
        return res;
    }
    
    // Only if current frame not the same as previous
    current = *element;
    current.count = 1;
    return 0;
  };
  
  if (!stacktraceEnabled)
    return -ENOSYS;
  
# if IS_ENABLED(CONFIG_STACKTRACE_PROVIDER_LIBBACKTRACE)
  stacktrace_libbacktrace_walk_through_stack(walkerBlock);
# else
  res = -ENOSYS;
  UNUSED(walkerBlock);
# endif
  return res;
}

