#ifndef _headers_1674914743_FluffyGC_completion
#define _headers_1674914743_FluffyGC_completion

#include <stdbool.h>
#include <stdatomic.h>

#include "event.h"

struct completion {
  struct event onComplete;
  unsigned int done;
};

int completion_init(struct completion* self);
void completion_cleanup(struct completion* self);
void completion_reset(struct completion* self);

void completion__complete_all(struct completion* self);
void completion__complete(struct completion* self);
void completion__wait_for_completion(struct completion* self);

#define complete_all(self) do { \
  completion__complete_all((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#define complete(self) do { \
  completion__complete((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#define wait_for_completion(self, mutex) do { \
  atomic_thread_fence(memory_order_acquire); \
  completion__wait_for_completion((self), (mutex)); \
  atomic_thread_fence(memory_order_release); \
} while (0)


#endif

