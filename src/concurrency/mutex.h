#ifndef _headers_1674909338_FluffyGC_mutex
#define _headers_1674909338_FluffyGC_mutex

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>

#include "attributes.h"
#include "concurrency/rwlock.h"

struct mutex {
  pthread_mutex_t mutex;
  bool inited;
  
  bool locked;
  struct rwlock ownerLock;
  pthread_t owner;
};

int mutex_init(struct mutex* self);
void mutex_cleanup(struct mutex* self);

ATTRIBUTE_USED()
static inline void mutex_lock(struct mutex* self) {
  pthread_mutex_lock(&self->mutex);
  rwlock_wrlock(&self->ownerLock);
  self->owner = pthread_self();
  self->locked = true;
  rwlock_unlock(&self->ownerLock);
  atomic_thread_fence(memory_order_acquire);
} 

ATTRIBUTE_USED()
static inline bool mutex_try_lock(struct mutex* self) {
  int res = pthread_mutex_trylock(&self->mutex);
  if (res == EBUSY)
    return false;
  
  rwlock_wrlock(&self->ownerLock);
  self->owner = pthread_self();
  self->locked = true;
  rwlock_unlock(&self->ownerLock);
  atomic_thread_fence(memory_order_acquire);
  return true;
} 

ATTRIBUTE_USED()
static inline void mutex_unlock(struct mutex* self) {
  atomic_thread_fence(memory_order_release);
  rwlock_wrlock(&self->ownerLock);
  self->locked = false;
  rwlock_unlock(&self->ownerLock);
  pthread_mutex_unlock(&self->mutex);
}

ATTRIBUTE_USED()
static inline bool mutex_is_owned_by_current(struct mutex* self) {
  rwlock_rdlock(&self->ownerLock);
  bool res = pthread_equal(self->owner, pthread_self());
  rwlock_unlock(&self->ownerLock);
  return res;
}

#endif

