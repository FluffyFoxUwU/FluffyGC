#ifndef _headers_1674912142_FluffyGC_rwlock
#define _headers_1674912142_FluffyGC_rwlock

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <stddef.h>

#include "attributes.h"
#include "concurrency/condition.h"

struct rwlock {
  bool inited;
  pthread_rwlock_t rwlock;

  int flags;
  atomic_bool writersWaiting;
  struct condition writerDone;
};

#define RWLOCK_FLAGS_PREFER_WRITER 0x01

#define RWLOCK_INITIALIZER { \
    .rwlock = PTHREAD_RWLOCK_INITIALIZER, \
    .inited = true, \
    .flags = 0 \
  }
#define RWLOCK_INITIALIZER_PREFER_WRITER { \
    .rwlock = PTHREAD_RWLOCK_INITIALIZER, \
    .inited = true, \
    .flags = RWLOCK_FLAGS_PREFER_WRITER, \
    .writerDone = CONDITION_INITIALIZER \
  }
#define RWLOCK_DEFINE(name) \
  struct rwlock name = RWLOCK_INITIALIZER

int rwlock_init(struct rwlock* self, int flags);
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
static inline bool rwlock_tryrdlock(struct rwlock* self) {
  if (pthread_rwlock_rdlock(&(self)->rwlock) == EBUSY)
    return false;
  atomic_thread_fence(memory_order_acquire);
  return true;
}

ATTRIBUTE_USED()
static inline bool rwlock_trywrlock(struct rwlock* self) {
  if (pthread_rwlock_wrlock(&(self)->rwlock) == EBUSY)
    return false;
  atomic_thread_fence(memory_order_acquire);
  return true;
}

ATTRIBUTE_USED()
static inline void rwlock_unlock(struct rwlock* self) {
  atomic_thread_fence(memory_order_release);
  pthread_rwlock_unlock(&(self)->rwlock);
}

#endif

