#ifndef _headers_1674913199_FluffyGC_condition
#define _headers_1674913199_FluffyGC_condition

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

struct mutex;
struct condition {
  pthread_cond_t cond;
  bool inited;
};

int condition_init(struct condition* self);
void condition_cleanup(struct condition* self);

void condition__wake(struct condition* self);
void condition__wake_all(struct condition* self);
void condition__wait(struct condition* self, struct mutex* mutex);

#define condition_wait(self, mutex) do { \
  atomic_thread_fence(memory_order_release); \
  condition__wait((self), (mutex)); \
  atomic_thread_fence(memory_order_acquire); \
} while (0)

#define condition_wake(self) do { \
  atomic_thread_fence(memory_order_release); \
  condition__wake((self)); \
} while (0)

#define condition_wake_all(self) do { \
  atomic_thread_fence(memory_order_release); \
  condition__wake_all((self)); \
} while (0)

#endif

