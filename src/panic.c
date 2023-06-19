#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "panic.h"

static atomic_bool hasPanic;

static pthread_once_t initControl = PTHREAD_ONCE_INIT;
static void init() {
  atomic_init(&hasPanic, false);
}

void _panic_va(const char* fmt, va_list list) {
  pthread_once(&initControl, init);
  
  // Double panic, sleep forever
  while (atomic_exchange(&hasPanic, true) == true)
    sleep(UINT32_MAX);
  
  // Using static buffer as panic might occur on OOM scenario
  static char panicBuffer[512 * 1024] = {0};
  
  size_t panicMsgLen = vsnprintf(panicBuffer, sizeof(panicBuffer), fmt, list);
  fprintf(stderr, "Program panic: %s\n", panicBuffer);
  if (panicMsgLen >= sizeof(panicBuffer)) 
    fprintf(stderr, "Panic message was truncated! (%zu bytes to %zu bytes)\n", panicMsgLen, sizeof(panicBuffer) - 1); 
  abort();
}

void _panic(const char* fmt, ...) {
  pthread_once(&initControl, init);
  
  va_list args;
  va_start(args, fmt);
  _panic_va(fmt, args);
  va_end(args);
}
