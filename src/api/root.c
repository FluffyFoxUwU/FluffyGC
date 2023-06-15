#include <stdatomic.h>

#include "pre_code.h"

#include "context.h"
#include "FluffyHeap.h"

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_dup_ref(__FLUFFYHEAP_NULLABLE(fh_object*) ref) {
  if (!ref)
    return NULL;
  context_block_gc();
  struct root_ref* new = context_add_root_object(atomic_load(&((struct root_ref*) ref)->obj));
  context_unblock_gc();
  return (fh_object*) new;
}

__FLUFFYHEAP_EXPORT void fh_del_ref(__FLUFFYHEAP_NONNULL(fh_object*) ref) {
  context_remove_root_object((struct root_ref*) ref);
}
