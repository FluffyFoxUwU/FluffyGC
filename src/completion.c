#include <errno.h>
#include <limits.h>

#include "completion.h"
#include "bug.h"
#include "event.h"
#include "mutex.h"

int completion_init(struct completion* self) {
  *self = (struct completion) {};
  if (event_init(&self->onComplete) < 0)
    return -ENOMEM;
  return 0;
}

void completion_cleanup(struct completion* self) {
  event_cleanup(&self->onComplete);
}

void completion__complete(struct completion* self) {
  mutex_lock(&self->onComplete.lock);
  BUG_ON(self->done == UINT_MAX);
  self->done++;
  event_fire_locked(&self->onComplete);
  mutex_unlock(&self->onComplete.lock);
}

void completion_reset(struct completion* self) {
  self->done = 0;
  event_reset(&self->onComplete);
}

void completion__complete_all(struct completion* self) {
  mutex_lock(&self->onComplete.lock);
  self->done = UINT_MAX;
  event_fire_all_locked(&self->onComplete);
  mutex_unlock(&self->onComplete.lock);
}

void completion__wait_for_completion(struct completion* self) {
  mutex_lock(&self->onComplete.lock);
  event_wait(&self->onComplete);
  if (self->done != UINT_MAX)
    self->done--;
  mutex_unlock(&self->onComplete.lock);
}
