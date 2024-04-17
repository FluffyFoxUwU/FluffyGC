#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

#include "bug.h"
#include "hook/hook.h"
#include "logger/logger.h"
#include "macros.h"
#include "panic.h"
#include "util/util.h"
#include "main.h"

// Runs forever
static void* loggerFunc(void*) {
  util_set_thread_name("Logger Thread");
  
  const char* logFilename = "./latest.log"; 
  FILE* logFile = NULL; //fopen(logFilename, "a");
  if (!logFile) {
    pr_error("Error opening '%s' log file, logs will not be logged", logFilename);
    pr_info("Logging into %s", logFilename);
  }
  
  bool isLogBuffered = false;
  if (logFile && setvbuf(logFile, NULL, _IONBF, 0) < 0) {
    pr_error("Error setting log buffering, relying on explicit fflush");
    isLogBuffered = true;
  }
  
  while (1) {
    struct logger_entry entry;
    logger_get_entry(&entry);
    fputs(entry.message, stderr);
    fputs("\n", stderr);
    if (logFile) {
      fputs(entry.message, logFile);
      fputs("\n", logFile);
    }
    
    if (isLogBuffered)
      fflush(logFile);
  }
  
  fclose(logFile);
  return NULL;
}

int main2(int argc, const char** argv) {
  UNUSED(argc);
  UNUSED(argv);

  util_set_thread_name("Main Thread");
  logger_init();
  
  pthread_t loggerThread;
  int ret = 0;
  if ((ret = -pthread_create(&loggerThread, NULL, loggerFunc, NULL)) < 0)
    hard_panic("Logger thread can't start: %d", ret);
  pr_alert("Starting UwU");
  panic("error");
  
  ret = hook_init();
  BUG_ON(ret < 0 && ret != -ENOSYS);
  
  return EXIT_SUCCESS;
}
