#include <errno.h>

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
  self->done++;
  event_fire_locked(&self->onComplete);
  mutex_unlock(&self->onComplete.lock);
}

void completion__wait_for_completion(struct completion* self) {
  mutex_lock(&self->onComplete.lock);
  event_wait(&self->onComplete);
  self->done--;
  mutex_unlock(&self->onComplete.lock);
}
