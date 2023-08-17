#ifndef _headers_1674909338_FluffyGC_mutex
#define _headers_1674909338_FluffyGC_mutex

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

struct mutex {
  pthread_mutex_t mutex;
  bool inited;
  
  // Must not be dereferenced
  atomic_uintptr_t owner;
};

#define MUTEX_INITIALIZER { \
    .mutex = PTHREAD_MUTEX_INITIALIZER, \
    .inited = true, \
    .owner = 0, \
  }
#define MUTEX_DEFINE(name) \
  struct mutex name = MUTEX_INITIALIZER;

int mutex_init(struct mutex* self);
void mutex_cleanup(struct mutex* self);

// nonblock takes over precedence to timed
#define MUTEX_LOCK_NONBLOCK 0x01
#define MUTEX_LOCK_TIMED    0x02

void mutex_lock(struct mutex* self);
int mutex_lock2(struct mutex* self, int flags, const struct timespec* abstime);

void mutex_unlock(struct mutex* self);
bool mutex_is_owned_by_current(struct mutex* self);

void mutex_mark_as_owner_by_current(struct mutex* self);
void mutex_unmark_as_owner_by_current(struct mutex* self);

#endif

