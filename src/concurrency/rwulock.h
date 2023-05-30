#ifndef _headers_1685448642_FluffyGC_rwulock
#define _headers_1685448642_FluffyGC_rwulock

#include "concurrency/mutex.h"
#include "concurrency/rwlock.h"

struct rwulock {
  struct rwlock lock;
  struct mutex upgradeLock;
};

static inline int rwulock_init(struct rwulock* self) {
  int res = rwlock_init(&self->lock);
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

static inline void rwulock_wrlock(struct rwulock* self) {
  mutex_lock(&self->upgradeLock);
  rwlock_wrlock(&self->lock);
}

// Only try upgrade available due concurrent upgrade
// can cause deadlocks
static inline bool rwulock_try_upgrade(struct rwulock* self) {
  if (!mutex_try_lock(&self->upgradeLock))
    return false;
  
  rwlock_unlock(&self->lock);
  rwlock_wrlock(&self->lock);
  return true;
}

static inline void rwulock_unlock(struct rwulock* self) {
  rwlock_unlock(&self->lock);
  if (mutex_is_owned_by_current(&self->upgradeLock))
    mutex_unlock(&self->upgradeLock);
}

#endif

