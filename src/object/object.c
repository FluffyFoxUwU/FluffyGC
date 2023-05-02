#include <stdatomic.h>
#include <errno.h>
#include <stddef.h>

#include "gc/gc.h"
#include "object.h"
#include "context.h"
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
  gc_read_barrier(obj);
obj_is_null:
  context_unblock_gc();
  return res;
}

void object_write_reference(struct object* self, size_t offset, struct object* obj) {
  context_block_gc();
  struct object* old = atomic_exchange(getAtomicPtrToReference(self, offset), obj);
  
  // TODO: implement conditional write barriers
  gc_write_barrier(old);
  context_unblock_gc();
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
  
  descriptor_init(desc, self);
}
