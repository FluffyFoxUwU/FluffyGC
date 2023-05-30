#ifndef _headers_1674912142_FluffyGC_rwlock
#define _headers_1674912142_FluffyGC_rwlock

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "attributes.h"

struct rwlock {
  pthread_rwlock_t rwlock;
  bool inited;
};

int rwlock_init(struct rwlock* self);
void rwlock_cleanup(struct rwlock* self);

ATTRIBUTE_USED()
static inline void rwlock_wrlock(struct rwlock* self) {
  pthread_rwlock_wrlock(&(self)->rwlock);
  atomic_thread_fence(memory_order_acquire);
}

ATTRIBUTE_USED()
static inline void rwlock_rdlock(struct rwlock* self) {
  pthread_rwlock_rdlock(&(self)->rwlock);
  atomic_thread_fence(memory_order_acquire);
}

ATTRIBUTE_USED()
static inline void rwlock_unlock(struct rwlock* self) {
  atomic_thread_fence(memory_order_release);
  pthread_rwlock_unlock(&(self)->rwlock);
}

#endif

