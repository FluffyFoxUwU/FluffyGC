#ifndef _headers_1674909338_FluffyGC_mutex
#define _headers_1674909338_FluffyGC_mutex

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

struct mutex {
  pthread_mutex_t mutex;
  bool inited;
};

int mutex_init(struct mutex* self);
void mutex_cleanup(struct mutex* self);

void mutex__lock(struct mutex* self);
void mutex__unlock(struct mutex* self);

#define mutex_lock(self) do { \
  atomic_thread_fence(memory_order_acquire); \
  mutex__lock((self)); \
} while (0)

#define mutex_unlock(self) do { \
  mutex__unlock((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#endif

