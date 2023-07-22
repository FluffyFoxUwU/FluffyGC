#include "api/pre_code.h"

#include "api/mods/debug/debug.h"
#include "hook/hook.h"
#include "FluffyHeap.h"
#include "managed_heap.h"
#include "object/descriptor.h"
#include "object/descriptor/array.h"
#include "object/object.h"
#include "util/util.h"

// Array related checks comes here

static bool checkIfArray(fh_object* self, const char* src) {
  context_block_gc();
  struct descriptor* desc = atomic_load(&INTERN(self)->obj)->movePreserve.descriptor;
  if (desc->type != OBJECT_ARRAY)
    debug_warn("%s: Getting length on non array object!!\n", src);
  return desc->type == OBJECT_ARRAY;
  context_unblock_gc();
}

HOOK_FUNCTION(, size_t, debug_hook_fh_array_get_length_head, __FLUFFYHEAP_NONNULL(fh_array*), self) {
  ci->action = HOOK_CONTINUE;
  debug_try_print_api_call(self);
  
  if (!debug_can_do_check())
    return;
  
  checkIfArray(EXTERN(INTERN(self)), __FUNCTION__);
  return;
}

HOOK_FUNCTION(, size_t, debug_hook_fh_array_get_length_tail, __FLUFFYHEAP_NONNULL(fh_array*), self) {
  ci->action = HOOK_CONTINUE;
  debug_try_print_api_return_value(*returnLocation);
}

HOOK_FUNCTION(, ssize_t, debug_hook_fh_array_calc_offset_head, __FLUFFYHEAP_NONNULL(fh_array*), self, size_t, index) {
  ci->action = HOOK_CONTINUE;
  debug_try_print_api_call(self, index);
  
  if (!debug_can_do_check())
    return;
  
  context_block_gc();
  struct object* obj = atomic_load(&INTERN(self)->obj);
  
  if (!checkIfArray(EXTERN(INTERN(self)), __FUNCTION__))
    goto not_array;
  
  struct array_descriptor* desc = container_of(obj->movePreserve.descriptor, struct array_descriptor, super);
  size_t len = desc->arrayInfo.length;
  
  if (index >= len)
    debug_info("%s: %s: Potential out of bound access! Tried to calculate offset for %zu in %zu length array", __FUNCTION__, object_get_unique_name(obj), index, len);
not_array:
  context_unblock_gc();
}

HOOK_FUNCTION(, ssize_t, debug_hook_fh_array_calc_offset_tail, __FLUFFYHEAP_NONNULL(fh_array*), self, size_t, index) {
  ci->action = HOOK_CONTINUE;
  debug_try_print_api_return_value(*returnLocation);
}
