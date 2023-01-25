#include <stdatomic.h>
#include <stddef.h>

#include "object.h"
#include "context.h"
#include "util.h"
#include "descriptor.h"
#include "bug.h"
#include "compiler_config.h"

ATTRIBUTE_USED()
const void* const object_failure_ptr = &(const struct object) {};

static object_ptr_atomic* getAtomicPtr(struct object* self, size_t offset) {
  return (object_ptr_atomic*) object_get_dma(self) + offset;
}

struct object* object_resolve_forwarding(struct object* self) {
  while (self->forwardingPointer)
    self = self->forwardingPointer;
  return self;
}

struct object* object_read_ptr(struct object* self, size_t offset) {
  context_block_gc();
  struct object* obj = atomic_load(getAtomicPtr(self, offset));
  if (!obj)
    goto obj_is_null;
  
  struct object* forwarded = object_resolve_forwarding(obj);
  
  if (obj != forwarded)
    atomic_compare_exchange_strong(getAtomicPtr(self, offset), &obj, forwarded);
  
  bool res = context_add_root_object(obj);
  if (!res)
    obj = OBJECT_FAILURE_PTR;
obj_is_null:
  context_unblock_gc();
  return obj;
}

void object_write_ptr(struct object* self, size_t offset, struct object* obj) {
  atomic_store(getAtomicPtr(self, offset), obj);
}

static void* calcDmaPtr(struct object* self) {
  return self->dataPtr;
}

void* object_get_dma(struct object* self) {
  context_block_gc();
  context_add_pinned_object(self);
  context_unblock_gc();
  
  return calcDmaPtr(self);
}

void object_put_dma(struct object* self, void* dma) {
  BUG_ON(dma != calcDmaPtr(self));
  context_block_gc();
  context_remove_pinned_object(self);
  context_unblock_gc();
}

void object_init(struct object* self, struct descriptor* desc, void* data) {
  *self = (struct object) {
    .descriptor = desc,
    .objectSize = desc->objectSize,
    .dataPtr = data
  };
  
  descriptor_init(desc, self);
}
