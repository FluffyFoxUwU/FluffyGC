#include <stdatomic.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "gc/gc.h"
#include "managed_heap.h"
#include "object.h"
#include "context.h"
#include "util/list_head.h"
#include "util/util.h"
#include "descriptor.h"
#include "bug.h"
#include "attributes.h"

static _Atomic(struct object*)* getAtomicPtrToReference(struct object* self, size_t offset) {
  return (_Atomic(struct object*)*) self->dataPtr.ptr + offset;
}

struct object* object_resolve_forwarding(struct object* self) {
  while (self->forwardingPointer)
    self = self->forwardingPointer;
  return self;
}

struct root_ref* object_read_reference(struct object* self, size_t offset) {
  context_block_gc();
  struct object* obj = atomic_load(getAtomicPtrToReference(self, offset));
  struct root_ref* res = NULL;
  if (!obj)
    goto obj_is_null;
  
  struct object* forwarded = object_resolve_forwarding(obj);
  
  // Replace the pointer field into forwarded pointer
  if (obj != forwarded)
    atomic_compare_exchange_strong(getAtomicPtrToReference(self, offset), &obj, forwarded);
  obj = forwarded;
  
  res = context_add_root_object(obj);
  // Called in blocked GC state to ensure GC cycle not starting before
  // obj is checked for liveness
  gc_current->hooks->postReadBarrier(obj);
obj_is_null:
  context_unblock_gc();
  return res;
}

void object_write_reference(struct object* self, size_t offset, struct object* obj) {
  context_block_gc();
  struct object* old = atomic_exchange(getAtomicPtrToReference(self, offset), obj);
  
  // TODO: implement conditional write barriers
  gc_current->hooks->postWriteBarrier(old);
  
  // Remembered set management
  if (self->generationID != obj->generationID)
    list_add(&self->list, &context_current->managedHeap->generations[self->generationID].rememberedSet);
  context_unblock_gc();
}

void object_cleanup(struct object* self) {
  list_del(&self->list);
  
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
  
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    list_init_as_invalid(&self->rememberedSetNode[i]);
  descriptor_init_object(desc, self);
}
