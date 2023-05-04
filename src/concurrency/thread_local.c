#include <pthread.h>
#include <stdlib.h>

#include "bug.h"
#include "thread_local.h"

int thread_local_init(struct thread_local_struct* self, void (*destructor)(void* udata)) {
  *self = (struct thread_local_struct) {};
  
  int ret = -pthread_key_create(&self->key, destructor);
  self->inited = true;
  return ret;
}

void thread_local_cleanup(struct thread_local_struct* self) {
  if (!self->inited)
    return;
  pthread_key_delete(self->key);
}

void* thread_local_get(struct thread_local_struct* self) {
  return pthread_getspecific(self->key);
}

void thread_local_set(struct thread_local_struct* self, void* udata) {
  int ret = pthread_setspecific(self->key, udata);
  BUG_ON(ret != 0);
}
