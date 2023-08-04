#include <errno.h>
#include <limits.h>

#include "completion.h"
#include "bug.h"
#include "concurrency/condition.h"
#include "mutex.h"

int completion_init(struct completion* self) {
  *self = (struct completion) {};
  if (condition_init(&self->onComplete) < 0)
    return -ENOMEM;
  return 0;
}

void completion_cleanup(struct completion* self) {
  mutex_cleanup(&self->lock);
  condition_cleanup(&self->onComplete);
}

void completion__complete(struct completion* self) {
  mutex_lock(&self->lock);
  BUG_ON(self->done == UINT_MAX);
  self->done++;
  condition_wake(&self->onComplete);
  mutex_unlock(&self->lock);
}

void completion_reset(struct completion* self) {
  self->done = 0;
}

void completion__complete_all(struct completion* self) {
  mutex_lock(&self->lock);
  self->done = UINT_MAX;
  condition_wake_all(&self->onComplete);
  mutex_unlock(&self->lock);
}

void completion__wait_for_completion(struct completion* self) {
  mutex_lock(&self->lock);
  
  condition_wait(&self->onComplete, &self->lock, ^bool () {
    return self->done == 0;
  });
  
  if (self->done != UINT_MAX)
    self->done--;
  mutex_unlock(&self->lock);
}
