#ifndef _headers_1674802032_FluffyGC_event
#define _headers_1674802032_FluffyGC_event

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "mutex.h"
#include "condition.h"

struct event {
  struct mutex lock;
  struct condition cond;
  bool fired;
};

int event_new(struct event* self);
void event_free(struct event* self);

void event__wait(struct event* self);
void event__fire(struct event* self);

#define event_wait(self) do { \
  atomic_thread_fence(memory_order_acquire); \
  event__wait((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#define event_fire(self) do { \
  atomic_thread_fence(memory_order_acquire); \
  event__fire((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#endif

