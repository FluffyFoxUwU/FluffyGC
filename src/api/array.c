#include "pre_code.h"

#include "object/descriptor/array.h"
#include "context.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "FluffyHeap.h"
#include "managed_heap.h"

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_array*) fh_alloc_array(__FLUFFYHEAP_NONNULL(fh_descriptor*) elementType, size_t length) {
  struct array_descriptor arrayDescriptor;
  if (array_descriptor_init(&arrayDescriptor, INTERN(elementType), length) < 0)
    return NULL;
  return CAST_TO_ARRAY(EXTERN(managed_heap_alloc_object(&arrayDescriptor.super)));
}

__FLUFFYHEAP_EXPORT ssize_t fh_array_calc_offset(__FLUFFYHEAP_NONNULL(fh_array*) self, size_t index) {
  context_block_gc();
  struct descriptor* desc = atomic_load(&INTERN(self)->obj)->movePreserve.descriptor;
  BUG_ON(desc->type != OBJECT_ARRAY);
  ssize_t res = descriptor_calc_offset(desc, index);
  context_unblock_gc();
  return res;
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_array_get_element(__FLUFFYHEAP_NONNULL(fh_array*) self, size_t index) {
  return fh_object_read_ref(FH_CAST_TO_OBJECT(self), fh_array_calc_offset(self, index));
}

__FLUFFYHEAP_EXPORT void fh_array_set_element(__FLUFFYHEAP_NONNULL(fh_array*) self, size_t index, __FLUFFYHEAP_NULLABLE(fh_object*) object) {
  fh_object_write_ref(FH_CAST_TO_OBJECT(self), fh_array_calc_offset(self, index), object);
}

