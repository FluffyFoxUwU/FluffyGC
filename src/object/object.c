#include <limits.h>
#include <stdatomic.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "gc/gc.h"
#include "managed_heap.h"
#include "memory/heap.h"
#include "object.h"
#include "context.h"
#include "util/list_head.h"
#include "util/util.h"
#include "descriptor.h"
#include "bug.h"
#include "attributes.h"
#include "vec.h"

static _Atomic(struct object*)* getAtomicPtrToReference(struct object* self, size_t offset) {
  return (_Atomic(struct object*)*) (self->dataPtr.ptr + offset);
}

struct object* object_resolve_forwarding(struct object* self) {
  while (self->forwardingPointer)
    self = self->forwardingPointer;
  return self;
}

static struct object* readPointerAt(struct object* self, size_t offset) {
  struct object* obj = atomic_load(getAtomicPtrToReference(self, offset));
  if (!obj)
    return NULL;
  
  struct object* forwarded = object_resolve_forwarding(obj);
  
  // Replace the pointer field into forwarded pointer
  if (obj != forwarded)
    atomic_compare_exchange_strong(getAtomicPtrToReference(self, offset), &obj, forwarded);
  obj = forwarded;
  return obj;
}

struct root_ref* object_read_reference(struct object* self, size_t offset) {
  context_block_gc();
  struct root_ref* res = NULL;
  struct object* obj = readPointerAt(self, offset);
  if (!obj)
    goto obj_is_null;
  
  res = context_add_root_object(obj);
  // Called in blocked GC state to ensure GC cycle not starting before
  // obj is checked for liveness
  gc_current->ops->postReadBarrier(obj);
obj_is_null:
  context_unblock_gc();
  return res;
}

void object_write_reference(struct object* self, size_t offset, struct object* obj) {
  context_block_gc();
  if (!descriptor_is_assignable_to(self->descriptor, offset, obj->descriptor))
    BUG();
  
  struct object* old = atomic_exchange(getAtomicPtrToReference(self, offset), obj);
  
  // TODO: implement conditional write barriers
  gc_current->ops->postWriteBarrier(old);
  
  // Add current object to `obj`'s generation remembered set
  if (obj && self->generationID != obj->generationID && !list_is_valid(&self->rememberedSetNode[obj->generationID])) {
    struct generation* target = &managed_heap_current->generations[obj->generationID];
    mutex_lock(&target->rememberedSetLock);
    list_add(&self->rememberedSetNode[obj->generationID], &target->rememberedSet);
    mutex_unlock(&target->rememberedSetLock);
  }
  context_unblock_gc();
}

void object_cleanup(struct object* self) {
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    if (list_is_valid(&self->rememberedSetNode[i]))
      list_del(&self->rememberedSetNode[i]);
}

struct userptr object_get_dma(struct root_ref* rootRef) {
  context_add_pinned_object(rootRef);
  return atomic_load(&rootRef->obj)->dataPtr;
}

int object_put_dma(struct root_ref* rootRef, struct userptr dma) {
  if (dma.ptr != atomic_load(&rootRef->obj)->dataPtr.ptr)
    return -EINVAL;
  context_remove_pinned_object(rootRef);
  return 0;
}

void object_init(struct object* self, struct descriptor* desc, void* data) {
  *self = (struct object) {
    .descriptor = desc,
    .objectSize = desc->objectSize,
    .dataPtr = {data}
  };
  
  atomic_init(&self->isMarked, false);
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    list_init_as_invalid(&self->rememberedSetNode[i]);
  descriptor_init_object(desc, self);
}

void object_fix_pointers(struct object* self) {
  BUG_ON(self->generationID < 0);
  
  object_for_each_field(self, ^(struct object*, size_t offset) {
    readPointerAt(self, offset);
  });
}

struct object* object_move(struct object* self, struct heap* dest) {
  struct heap_block* newBlock = heap_alloc(dest, self->descriptor->alignment, self->objectSize);
  if (!newBlock)
    return NULL;
  
  struct object* newBlockObj = &newBlock->objMetadata;
  self->forwardingPointer = newBlockObj;
  object_init(newBlockObj, self->descriptor, newBlock->dataPtr.ptr);
  memcpy(newBlockObj->dataPtr.ptr, self->dataPtr.ptr, self->objectSize);
  
  newBlockObj->generationID = self->generationID;
  newBlockObj->descriptor = self->descriptor;
  newBlockObj->age = self->age;
  
  // Poison self
  self->generationID = INT_MIN;
  //self->dataPtr = USERPTR_NULL;
  self->descriptor = NULL;
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    struct heap_block* block = container_of(self, struct heap_block, objMetadata);
    free(block->dataPtr.ptr);
    block->dataPtr.ptr = NULL;
  }
  return newBlockObj;
}

void object_for_each_field(struct object* self, void (^iterator)(struct object* obj,size_t offset)) {
  descriptor_for_each_field(self, iterator);
}
