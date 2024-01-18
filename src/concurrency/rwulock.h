#ifndef _headers_1685448642_FluffyGC_rwulock
#define _headers_1685448642_FluffyGC_rwulock

#include "concurrency/mutex.h"
#include "concurrency/rwlock.h"
#include <stddef.h>
#include <stdatomic.h>

struct rwulock {
  struct rwlock lock;
  struct mutex upgradeLock;
};

#define RWULOCK_INITIALIZER { \
    .lock = RWLOCK_INITIALIZER, \
    .upgradeLock = MUTEX_INITIALIZER \
  }
#define RWULOCK_DEFINE(name) \
  struct rwulock name = RWULOCK_INITIALIZER

static inline int rwulock_init(struct rwulock* self) {
  int res = rwlock_init(&self->lock, 0);
  if (res < 0)
    return res;
  return mutex_init(&self->upgradeLock);
}

static inline void rwulock_cleanup(struct rwulock* self) {
  rwlock_cleanup(&self->lock);
  mutex_cleanup(&self->upgradeLock);
}

static inline void rwulock_rdlock(struct rwulock* self) {
  rwlock_rdlock(&self->lock);
}

static inline bool rwulock_tryrdlock(struct rwulock* self) {
  if (!rwlock_tryrdlock(&self->lock))
    return false;
  atomic_thread_fence(memory_order_acquire);
  return true;
}

static inline void rwulock_wrlock(struct rwulock* self) {
  mutex_lock(&self->upgradeLock);
  rwlock_wrlock(&self->lock);
}

static inline bool rwulock_trywrlock(struct rwulock* self) {
  if (!rwlock_trywrlock(&self->lock))
    return false;
  atomic_thread_fence(memory_order_acquire);
  return true;
}

// Only try upgrade available due concurrent upgrade
// can cause deadlocks
static inline bool rwulock_try_upgrade(struct rwulock* self) {
  if (mutex_lock2(&self->upgradeLock, MUTEX_LOCK_NONBLOCK, NULL) < 0)
    return false;
  
  rwlock_unlock(&self->lock);
  rwlock_wrlock(&self->lock);
  return true;
}

static inline void rwulock_downgrade(struct rwulock* self) {
  rwlock_unlock(&self->lock);
  mutex_unlock(&self->upgradeLock);
  rwlock_rdlock(&self->lock);
}

static inline void rwulock_unlock(struct rwulock* self) {
  rwlock_unlock(&self->lock);
  if (mutex_is_owned_by_current(&self->upgradeLock))
    mutex_unlock(&self->upgradeLock);
}

#endif

