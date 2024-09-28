#define _GNU_SOURCE

#include <unistd.h>
#include <sys/syscall.h>

#include "thread_id.h"

static thread_local bool isCurrentTIDValid = false;
static thread_local pid_t currentTID;

pid_t platform_linux_gettid() {
  if (!isCurrentTIDValid) {
    currentTID = (pid_t) syscall(SYS_gettid);
  }
  
  return currentTID;
}


