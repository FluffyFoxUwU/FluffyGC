#include <errno.h>
#include <pthread.h>

#include "mutex.h"
#include "bug.h"

int mutex_init(struct mutex* self) {
  *self = (struct mutex) {};
  self->inited = false;
  if (pthread_mutex_init(&self->mutex, NULL) != 0)
    return -ENOMEM;
  self->inited = true;
  return 0;
}

void mutex_cleanup(struct mutex* self) {
  if (!self)
    return;
  if (self->inited && pthread_mutex_destroy(&self->mutex) != 0)
    BUG();
}

void mutex__lock(struct mutex* self) {
  pthread_mutex_lock(&self->mutex);
}

void mutex__unlock(struct mutex* self) {
  pthread_mutex_unlock(&self->mutex);
}
