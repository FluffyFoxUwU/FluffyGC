#include <errno.h>
#include <pthread.h>

#include "concurrency/rwlock.h"
#include "mutex.h"
#include "bug.h"

int mutex_init(struct mutex* self) {
  *self = (struct mutex) {};
  self->locked = false;
  self->inited = false;
  int res = 0;
  if ((res = pthread_mutex_init(&self->mutex, NULL)) != 0)
    return res == ENOMEM ? -ENOMEM : -EAGAIN;
  self->inited = true;
  return rwlock_init(&self->ownerLock);
}

void mutex_cleanup(struct mutex* self) {
  if (!self)
    return;
  if (self->inited && pthread_mutex_destroy(&self->mutex) != 0)
    BUG();
  rwlock_cleanup(&self->ownerLock);
}
