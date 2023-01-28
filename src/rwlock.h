#ifndef _headers_1674912142_FluffyGC_rwlock
#define _headers_1674912142_FluffyGC_rwlock

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

struct rwlock {
  pthread_rwlock_t rwlock;
  bool inited;
};

int rwlock_init(struct rwlock* self);
void rwlock_cleanup(struct rwlock* self);

void rwlock__rdlock(struct rwlock* self);
void rwlock__wrlock(struct rwlock* self);
void rwlock__unlock(struct rwlock* self);

#define rwlock_rdlock(self) do { \
  atomic_thread_fence(memory_order_acquire); \
  rwlock__rdlock((self)); \
} while (0)

#define rwlock_wrlock(self) do { \
  atomic_thread_fence(memory_order_acquire); \
  rwlock__wrlock((self)); \
} while (0)

#define rwlock_unlock(self) do { \
  rwlock__unlock((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#endif

