#include <pthread.h>
#include <errno.h>

#include "bug.h"
#include "condition.h"
#include "event.h"
#include "mutex.h"

int event_new(struct event* self) {
  *self = (struct event) {};
  
  if (mutex_init(&self->lock) < 0 || condition_init(&self->cond) < 0)
    return -ENOMEM;
  return 0;
}

void event_free(struct event* self) {
  mutex_cleanup(&self->lock);
  condition_cleanup(&self->cond);
}

void event__wait(struct event* self) {
  mutex_lock(&self->lock);
  self->fired = false;
  while (!self->fired)
    condition_wait(&self->cond, &self->lock);
  self->fired = false;
  mutex_unlock(&self->lock);
}

void event__fire(struct event* self) {
  mutex_lock(&self->lock);
  self->fired = true;
  mutex_unlock(&self->lock);
  
  condition_wake(&self->cond);
}
