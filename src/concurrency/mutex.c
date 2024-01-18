#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#include "mutex.h"
#include "bug.h"
#include "panic.h"

int mutex_init(struct mutex* self) {
  *self = (struct mutex) {};
  self->inited = false;
  int res = 0;
  if ((res = pthread_mutex_init(&self->mutex, NULL)) != 0)
    return res == ENOMEM ? -ENOMEM : -EAGAIN;
  self->inited = true;
  return 0;
}

void mutex_cleanup(struct mutex* self) {
  if (!self)
    return;
  if (self->inited && pthread_mutex_destroy(&self->mutex) != 0)
    panic();
}

static thread_local char currentThreadIJustWannaTakeTheAddressUwU = '\0';

void mutex_lock(struct mutex* self) {
  mutex_lock2(self, 0, NULL);
}

int mutex_lock2(struct mutex* self, int flags, const struct timespec* abstime) {
  BUG_ON((flags & MUTEX_LOCK_NONBLOCK) && (flags & MUTEX_LOCK_TIMED));
  int ret;
  if (flags & MUTEX_LOCK_NONBLOCK)
    ret = pthread_mutex_trylock(&self->mutex);
  else if (flags & MUTEX_LOCK_TIMED)
    ret = pthread_mutex_timedlock(&self->mutex, abstime);
  else
    ret = pthread_mutex_lock(&self->mutex);
  
  switch (ret) {
    case EINVAL:
      return -EINVAL;
    case EBUSY:
      return -EBUSY;
    case ETIMEDOUT:
      return -ETIMEDOUT;
    case ENOTRECOVERABLE:
      panic("Why are ENOTRECOVERABLE even exists for pthread_mutex_*lock()");
  }
  
  mutex_mark_as_owner_by_current(self);
  return 0;
}

void mutex_unlock(struct mutex* self) {
  mutex_unmark_as_owner_by_current(self);
  pthread_mutex_unlock(&self->mutex);
}

bool mutex_is_owned_by_current(struct mutex* self) {
  return atomic_load(&self->owner) == (uintptr_t) &currentThreadIJustWannaTakeTheAddressUwU;
}

void mutex_mark_as_owner_by_current(struct mutex* self) {
  //atomic_store(&self->owner, (uintptr_t) &currentThreadIJustWannaTakeTheAddressUwU);
  uintptr_t expected = 0;
  if (!atomic_compare_exchange_strong(&self->owner, &expected, (uintptr_t) &currentThreadIJustWannaTakeTheAddressUwU))
    BUG();
}

void mutex_unmark_as_owner_by_current(struct mutex* self) {
  //atomic_store(&self->owner, (uintptr_t) 0);
  uintptr_t expected = (uintptr_t) &currentThreadIJustWannaTakeTheAddressUwU;
  if (!atomic_compare_exchange_strong(&self->owner, &expected, 0))
    BUG();
}
