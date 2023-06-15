#include <errno.h>

#include "pre_code.h"

#include "concurrency/mutex.h"
#include "managed_heap.h"
#include "FluffyHeap.h"
#include "context.h"

__FLUFFYHEAP_EXPORT int fh_set_current(__FLUFFYHEAP_NULLABLE(fh_context*) _context) {
  int ret = 0;
  struct context* context = INTERN(_context);
  struct managed_heap* managedHeap = context->managedHeap;
  mutex_lock(&managedHeap->contextTrackerLock);
  if (context->state == CONTEXT_RUNNING) {
    ret = -EBUSY;
    goto context_is_busy;
  }
  
  context_current = context;
  
context_is_busy:
  mutex_unlock(&managedHeap->contextTrackerLock);
  return ret;
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_context*) fh_get_current() {
  return EXTERN(context_current);
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_context*) fh_new_context(__FLUFFYHEAP_NONNULL(fluffyheap*) heap) {
  return EXTERN(managed_heap_new_context(INTERN(heap)));
}

__FLUFFYHEAP_EXPORT void fh_free_context(__FLUFFYHEAP_NONNULL(fluffyheap*) heap, __FLUFFYHEAP_NONNULL(fh_context*) self) {
  managed_heap_free_context(INTERN(heap), INTERN(self));
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NONNULL(fluffyheap*) fh_context_get_heap(__FLUFFYHEAP_NONNULL(fh_context*) self) {
  return EXTERN(INTERN(self)->managedHeap);
}
