#include "api.h"
#include "managed_heap.h"
#include "FluffyHeap.h"
#include "context.h"

API_FUNCTION_DEFINE(int, fh_set_current, __FLUFFYHEAP_NONNULL(fh_context*), _context) {
  return managed_heap_swap_context(INTERN(_context));
}

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_context*), fh_get_current) {
  return EXTERN(context_current);
}

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_context*), fh_new_context, __FLUFFYHEAP_NONNULL(fluffyheap*), heap) {
  return EXTERN(managed_heap_new_context(INTERN(heap)));
}

API_FUNCTION_DEFINE_VOID(fh_free_context, __FLUFFYHEAP_NONNULL(fluffyheap*), heap, __FLUFFYHEAP_NONNULL(fh_context*), self) {
  managed_heap_free_context(INTERN(heap), INTERN(self));
}

API_FUNCTION_DEFINE(__FLUFFYHEAP_NONNULL(fluffyheap*), fh_context_get_heap, __FLUFFYHEAP_NONNULL(fh_context*), self) {
  return EXTERN(INTERN(self)->managedHeap);
}
