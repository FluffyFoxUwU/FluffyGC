#include <pthread.h>
#include <errno.h>

#include "bug.h"
#include "condition.h"
#include "event.h"
#include "mutex.h"

int event_init(struct event* self) {
  *self = (struct event) {};
  
  int ret = 0;
  if (mutex_init(&self->lock) < 0 || condition_init(&self->cond) < 0) {
    ret = -ENOMEM;
    event_cleanup(self);
  }
  return ret;
}

void event_cleanup(struct event* self) {
  mutex_cleanup(&self->lock);
  condition_cleanup(&self->cond);
}

void event__wait(struct event* self) {
  self->fired = false;
  while (!self->fired)
    condition_wait(&self->cond, &self->lock);
  self->fired = false;
}

void event__fire(struct event* self) {
  mutex_lock(&self->lock);
  event_fire_locked(self);
  mutex_unlock(&self->lock);
}

void event__fire_locked(struct event* self) {
  self->fired = true;
  condition_wake(&self->cond);
}
