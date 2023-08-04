#include <errno.h>
#include <pthread.h>

#include "mutex.h"
#include "condition.h"
#include "panic.h"

int condition_init(struct condition* self) {
  *self = (struct condition) {};
  
  self->inited = false;
  int res = 0;
  if ((res = pthread_cond_init(&self->cond, NULL)) != 0)
    return res == ENOMEM ? -ENOMEM : -EAGAIN;
  self->inited = true;
  return 0;
}

void condition_cleanup(struct condition* self) {
  if (!self)
    return;
  if (self->inited && pthread_cond_destroy(&self->cond) != 0)
    panic();
}

void condition_wake(struct condition* self) {
  pthread_cond_signal(&self->cond);
}

void condition_wake_all(struct condition* self) {
  pthread_cond_broadcast(&self->cond);
}

int condition_wait2(struct condition* self, struct mutex* mutex, int flags, const struct timespec* abstimeout, condition_checker checker) {
  // Exit early if not needed
  if (!checker())
    return 0;
  
  int ret;
retry:
  if (flags & CONDITION_WAIT_TIMED)
    ret = pthread_cond_timedwait(&self->cond, &mutex->mutex, abstimeout);
  else
    ret = pthread_cond_wait(&self->cond, &mutex->mutex);
  
  switch (ret) {
    case EINVAL:
      return -EINVAL;
    case ETIMEDOUT:
      return -ETIMEDOUT;
    case 0:
      break;
    default:
      panic();
    
    case ENOTRECOVERABLE:
      panic("Why are ENOTRECOVERABLE even exists for pthread_cond_timedwait()");
  }
  
  // If only checker is enabled and checker says retry
  // jump to retry
  if (!(flags & CONDITION_WAIT_NO_CHECKER) && checker())
    goto retry;
  return 0;
}

void condition_wait(struct condition* self, struct mutex* mutex, condition_checker checker) {
  condition_wait2(self, mutex, 0, NULL, checker);
}
