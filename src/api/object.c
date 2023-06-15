#include <stdatomic.h>
#include <string.h>

#include "pre_code.h"

#include "FluffyHeap.h"
#include "concurrency/condition.h"
#include "context.h"
#include "managed_heap.h"
#include "object/object.h"

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_alloc_object(__FLUFFYHEAP_NONNULL(fh_descriptor*) desc) {
  return (fh_object*) managed_heap_alloc_object((struct descriptor*) desc);
}

__FLUFFYHEAP_EXPORT void fh_object_read_data(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(void*) buffer, size_t offset, size_t size) {
  context_block_gc();
  struct object* obj = atomic_load(&INTERN(self)->obj);
  memcpy(buffer, obj->dataPtr.ptr + offset, size);
  context_unblock_gc();
}

__FLUFFYHEAP_EXPORT void fh_object_write_data(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(const void*) buffer, size_t offset, size_t size) {
  context_block_gc();
  struct object* obj = atomic_load(&INTERN(self)->obj);
  memcpy(obj->dataPtr.ptr + offset, buffer, size);
  context_unblock_gc();
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_object_read_ref(__FLUFFYHEAP_NONNULL(fh_object*) self, size_t offset) {
  context_block_gc();
  fh_object* ret = EXTERN(object_read_reference(atomic_load(&INTERN(self)->obj), offset));
  context_unblock_gc();
  return ret;
}

__FLUFFYHEAP_EXPORT void fh_object_write_ref(__FLUFFYHEAP_NONNULL(fh_object*) self, size_t offset, __FLUFFYHEAP_NULLABLE(fh_object*) data) {
  context_block_gc();
  object_write_reference(atomic_load(&INTERN(self)->obj), offset, atomic_load(&INTERN(data)->obj));
  context_unblock_gc();
}

__FLUFFYHEAP_EXPORT int fh_init_synchronization_structs(__FLUFFYHEAP_NONNULL(fh_object*) self) {
  context_block_gc();
  int ret = object_init_synchronization_structs(atomic_load(&INTERN(self)->obj));
  context_unblock_gc();
  return ret;
}

__FLUFFYHEAP_EXPORT void fh_object_wait(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NULLABLE(const struct timespec*) timeout) {
  context_block_gc();
  struct object_sync_structure* syncStructure = atomic_load(&INTERN(self)->obj)->syncStructure;
  context_unblock_gc();
  
  managed_heap_set_context_state(context_current, CONTEXT_SLEEPING);
  condition_wait(&syncStructure->cond, &syncStructure->lock);
  managed_heap_set_context_state(context_current, CONTEXT_RUNNING);
}

__FLUFFYHEAP_EXPORT void fh_object_wake(__FLUFFYHEAP_NONNULL(fh_object*) self) {
  context_block_gc();
  condition_wake(&(atomic_load(&INTERN(self)->obj))->syncStructure->cond);
  context_unblock_gc();
}

__FLUFFYHEAP_EXPORT void fh_object_wake_all(__FLUFFYHEAP_NONNULL(fh_object*) self) {
  context_block_gc();
  condition_wake_all(&(atomic_load(&INTERN(self)->obj))->syncStructure->cond);
  context_unblock_gc();
}

__FLUFFYHEAP_EXPORT void fh_object_lock(__FLUFFYHEAP_NONNULL(fh_object*) self) {
  context_block_gc();
  struct object_sync_structure* syncStructure = atomic_load(&INTERN(self)->obj)->syncStructure;
  context_unblock_gc();
  
  managed_heap_set_context_state(context_current, CONTEXT_SLEEPING);
  mutex_lock(&syncStructure->lock);
  managed_heap_set_context_state(context_current, CONTEXT_RUNNING);
}

__FLUFFYHEAP_EXPORT void fh_object_unlock(__FLUFFYHEAP_NONNULL(fh_object*) self) {
  context_block_gc();
  struct object_sync_structure* syncStructure = atomic_load(&INTERN(self)->obj)->syncStructure;
  mutex_unlock(&syncStructure->lock);
  context_unblock_gc();
}

__FLUFFYHEAP_EXPORT bool fh_object_is_alias(__FLUFFYHEAP_NULLABLE(fh_object*) a, __FLUFFYHEAP_NULLABLE(fh_object*) b) {
  context_block_gc();
  bool ret = atomic_load(&INTERN(a)->obj) == atomic_load(&INTERN(b)->obj);
  context_unblock_gc();
  return ret;
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NONNULL(fh_descriptor*) fh_object_get_descriptor(__FLUFFYHEAP_NONNULL(fh_object*) self);
