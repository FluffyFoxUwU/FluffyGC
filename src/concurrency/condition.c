#include <errno.h>
#include <pthread.h>

#include "mutex.h"
#include "condition.h"
#include "bug.h"

int condition_init(struct condition* self) {
  *self = (struct condition) {};
  
  self->inited = false;
  int res = 0;
  if ((res = pthread_cond_init(&self->cond, NULL)) != 0)
    return res == ENOMEM ? -ENOMEM : -EAGAIN;
  self->inited = true;
  return 0;
}

void condition_cleanup(struct condition* self) {
  if (!self)
    return;
  if (self->inited && pthread_cond_destroy(&self->cond) != 0)
    BUG();
}

void condition__wake(struct condition* self) {
  pthread_cond_signal(&self->cond);
}

void condition__wake_all(struct condition* self) {
  pthread_cond_broadcast(&self->cond);
}

void condition__wait(struct condition* self, struct mutex* mutex) {
  pthread_cond_wait(&self->cond, &mutex->mutex);
}

