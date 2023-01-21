#include <stdatomic.h>
#include <stddef.h>

#include "object.h"
#include "thread.h"
#include "util.h"
#include "bug.h"
#include "compiler_config.h"

ATTRIBUTE_USED()
const void* const object_failure_ptr = &(const struct object) {};

static size_t getRedzoneSize(struct object* self) {
  return 0;
}

static object_ptr_atomic* getAtomicPtr(struct object* self, size_t offset) {
  return (object_ptr_atomic*) object_get_dma(self) + offset;
}

struct object* object_resolve_forwarding(struct object* self) {
  while (self->forwardingPointer)
    self = self->forwardingPointer;
  return self;
}

struct object* object_read_ptr(struct object* self, size_t offset) {
  thread_block_gc();
  struct object* obj = atomic_load(getAtomicPtr(self, offset));
  if (!obj)
    goto obj_is_null;
  
  struct object* forwarded = object_resolve_forwarding(obj);
  
  if (obj != forwarded)
    atomic_compare_exchange_strong(getAtomicPtr(self, offset), &obj, forwarded);
  
  bool res = thread_add_root_object(obj);
  if (!res)
    obj = OBJECT_FAILURE_PTR;
obj_is_null:
  thread_unblock_gc();
  return obj;
}

void object_write_ptr(struct object* self, size_t offset, struct object* obj) {
  atomic_store(getAtomicPtr(self, offset), obj);
}

static void* calcDmaPtr(struct object* self) {
  return self->payload + getRedzoneSize(self);
}

void* object_get_dma(struct object* self) {
  thread_block_gc();
  thread_add_pinned_object(self);
  thread_unblock_gc();
  
  return calcDmaPtr(self);
}

void object_put_dma(struct object* self, void* dma) {
  BUG_ON(dma != calcDmaPtr(self));
  thread_block_gc();
  thread_remove_pinned_object(self);
  thread_unblock_gc();
}

