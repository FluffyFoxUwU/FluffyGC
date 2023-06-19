#include <errno.h>
#include <pthread.h>

#include "panic.h"
#include "rwlock.h"

int rwlock_init(struct rwlock* self) {
  self->inited = false;
  if (pthread_rwlock_init(&self->rwlock, NULL) != 0)
    return -ENOMEM;
  self->inited = true;
  return 0;
}

void rwlock_cleanup(struct rwlock* self) {
  if (!self)
    return;
  if (self->inited && pthread_rwlock_destroy(&self->rwlock) != 0)
    panic();
}

void rwlock__rdlock(struct rwlock* self) {
  pthread_rwlock_rdlock(&self->rwlock);
}

void rwlock__wrlock(struct rwlock* self) {
  pthread_rwlock_wrlock(&self->rwlock);
}

void rwlock__unlock(struct rwlock* self) {
  pthread_rwlock_unlock(&self->rwlock);
}
