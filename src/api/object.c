#include "api/api.h"
#include "object/descriptor.h"
#include "object/descriptor/array.h"
#include "panic.h"

#include "FluffyHeap/FluffyHeap.h"
#include "concurrency/condition.h"
#include "context.h"
#include "managed_heap.h"
#include "object/object.h"

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_object*), fh_alloc_object, __FLUFFYHEAP_NONNULL(fh_descriptor*), desc) {
  return EXTERN(managed_heap_alloc_object(INTERN(desc)));
}

API_FUNCTION_DEFINE_VOID(fh_object_read_data, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(void*), buffer, size_t, offset, size_t, size) {
  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  object_ptr_use_start(obj);
  memcpy(buffer, (char*) object_get_ptr(obj) + offset, size);
  object_ptr_use_end(obj);
  context_unblock_gc();
}

API_FUNCTION_DEFINE_VOID(fh_object_write_data, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(const void*), buffer, size_t, offset, size_t, size) {
  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  object_ptr_use_start(obj);
  memcpy((char*) object_get_ptr(obj) + offset, buffer, size);
  object_ptr_use_end(obj);
  context_unblock_gc();
}

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_object*), fh_object_read_ref, __FLUFFYHEAP_NONNULL(fh_object*), self, size_t, offset) {
  context_block_gc();
  fh_object* ret = EXTERN(object_read_reference(root_ref_get(INTERN(self)), offset));
  context_unblock_gc();
  return ret;
}

API_FUNCTION_DEFINE_VOID(fh_object_write_ref, __FLUFFYHEAP_NONNULL(fh_object*), self, size_t, offset, __FLUFFYHEAP_NULLABLE(fh_object*), data) {
  context_block_gc();
  object_write_reference(root_ref_get(INTERN(self)), offset, root_ref_get(INTERN(data)));
  context_unblock_gc();
}

API_FUNCTION_DEFINE(int, fh_object_init_synchronization_structs, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  int ret = object_init_synchronization_structs(root_ref_get(INTERN(self)));
  context_unblock_gc();
  return ret;
}

API_FUNCTION_DEFINE_VOID(fh_object_wait, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NULLABLE(const struct timespec*), timeout) {
  context_block_gc();
  struct object_sync_structure* syncStructure = root_ref_get(INTERN(self))->movePreserve.syncStructure;
  context_unblock_gc();
  
  managed_heap_set_context_state(context_current, CONTEXT_SLEEPING);
  condition_wait2(&syncStructure->cond, &syncStructure->lock, CONDITION_WAIT_NO_CHECKER, timeout, NULL);
  managed_heap_set_context_state(context_current, CONTEXT_RUNNING);
}

API_FUNCTION_DEFINE_VOID(fh_object_wake, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  struct object_sync_structure* syncStructure = root_ref_get(INTERN(self))->movePreserve.syncStructure;
  context_unblock_gc();
  
  condition_wake(&syncStructure->cond);
}

API_FUNCTION_DEFINE_VOID(fh_object_wake_all, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  struct object_sync_structure* syncStructure = root_ref_get(INTERN(self))->movePreserve.syncStructure;
  context_unblock_gc();
  
  condition_wake_all(&syncStructure->cond);
}

API_FUNCTION_DEFINE_VOID(fh_object_lock, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  struct object_sync_structure* syncStructure = root_ref_get(INTERN(self))->movePreserve.syncStructure;
  context_unblock_gc();
  
  managed_heap_set_context_state(context_current, CONTEXT_SLEEPING);
  mutex_lock(&syncStructure->lock);
  managed_heap_set_context_state(context_current, CONTEXT_RUNNING);
}

API_FUNCTION_DEFINE_VOID(fh_object_unlock, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  struct object_sync_structure* syncStructure = root_ref_get(INTERN(self))->movePreserve.syncStructure;
  context_unblock_gc();
  mutex_unlock(&syncStructure->lock);
}

API_FUNCTION_DEFINE(bool, fh_object_is_alias, __FLUFFYHEAP_NULLABLE(fh_object*), a, __FLUFFYHEAP_NULLABLE(fh_object*), b) {
  context_block_gc();
  bool ret = root_ref_get(INTERN(a)) == root_ref_get(INTERN(b));
  context_unblock_gc();
  return ret;
}

API_FUNCTION_DEFINE(__FLUFFYHEAP_NONNULL(const fh_type_info*), fh_object_get_type_info, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  descriptor_acquire(obj->movePreserve.descriptor);
  struct descriptor* desc = obj->movePreserve.descriptor;
  context_unblock_gc();
  return &desc->api.typeInfo;
}

API_FUNCTION_DEFINE_VOID(fh_object_put_type_info, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(const fh_type_info*), typeInfo) {
  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  switch (typeInfo->type) {
    case FH_TYPE_NORMAL:
      BUG_ON(obj->movePreserve.descriptor != INTERN(typeInfo->info.normal));
      break;
    case FH_TYPE_ARRAY:
      BUG_ON(obj->movePreserve.descriptor != &container_of(typeInfo->info.refArray, struct array_descriptor, api.refArrayInfo)->super);
      break;
    case FH_TYPE_COUNT:
      panic();
  }
  descriptor_release(obj->movePreserve.descriptor);
  context_unblock_gc();
}

API_FUNCTION_DEFINE(fh_object_type, fh_object_get_type, __FLUFFYHEAP_NONNULL(fh_object*), self) {
  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  fh_object_type type = obj->movePreserve.descriptor->api.typeInfo.type;
  context_unblock_gc();
  return type;
}
