#ifndef _headers_1674914743_FluffyGC_completion
#define _headers_1674914743_FluffyGC_completion

#include <stdbool.h>
#include <stdatomic.h>

#include "concurrency/condition.h"
#include "concurrency/mutex.h"

struct completion {
  struct mutex lock;
  struct condition onComplete;
  unsigned int done;
};

int completion_init(struct completion* self);
void completion_cleanup(struct completion* self);
void completion_reset(struct completion* self);

void completion__complete_all(struct completion* self);
void completion__complete(struct completion* self);
void completion__wait_for_completion(struct completion* self);

#define complete_all(self) do { \
  atomic_thread_fence(memory_order_release); \
  completion__complete_all((self)); \
} while (0)

#define complete(self) do { \
  atomic_thread_fence(memory_order_release); \
  completion__complete((self)); \
} while (0)

#define wait_for_completion(self) do { \
  atomic_thread_fence(memory_order_release); \
  completion__wait_for_completion((self)); \
  atomic_thread_fence(memory_order_acquire); \
} while (0)


#endif

