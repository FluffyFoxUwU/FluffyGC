#include <stddef.h>
#include <errno.h>
#include <pthread.h>

#include "concurrency/condition.h"
#include "panic.h"
#include "rwlock.h"

int rwlock_init(struct rwlock* self, int flags) {
  int ret = 0;

  self->inited = false;
  self->flags = flags;
  if (pthread_rwlock_init(&self->rwlock, NULL) != 0) {
    ret = -ENOMEM;
    goto failure;
  }

  if (flags & RWLOCK_FLAGS_PREFER_WRITER)
    if ((ret = condition_init(&self->writerDone)) < 0)
      goto failure;
  self->inited = true;
  return 0;

failure:
  rwlock_cleanup(self);
  return ret;
}

void rwlock_cleanup(struct rwlock* self) {
  if (!self)
    return;
  if (self->flags & RWLOCK_FLAGS_PREFER_WRITER)
    condition_cleanup(&self->writerDone);
  if (self->inited && pthread_rwlock_destroy(&self->rwlock) != 0)
    panic();
}

