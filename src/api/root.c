#include <stdatomic.h>

#include "pre_code.h"

#include "api/api.h"
#include "context.h"
#include "FluffyHeap.h"

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_object*), fh_dup_ref, __FLUFFYHEAP_NULLABLE(fh_object*), ref) {
  if (!ref)
    return NULL;
  context_block_gc();
  struct root_ref* new = context_add_root_object(atomic_load(&INTERN(ref)->obj));
  context_unblock_gc();
  return EXTERN(new);
}

API_FUNCTION_DEFINE_VOID(fh_del_ref, __FLUFFYHEAP_NONNULL(fh_object*), ref) {
  context_remove_root_object(INTERN(ref));
}
