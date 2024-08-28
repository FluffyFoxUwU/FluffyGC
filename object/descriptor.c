#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

#include "descriptor.h"

void descriptor_init_object(struct descriptor* desc, size_t extraSize, void* data) {
  for (size_t i = 0; i < desc->fieldCount; i++) {
    _Atomic(struct arena_block*)* fieldPtr = (_Atomic(struct arena_block*)*) ((void*) (((char*) data) + desc->fields[i].offset));
    atomic_store_explicit(fieldPtr, NULL, memory_order_relaxed);
  }
  
  if (!desc->hasFlexArrayField)
    return;
  
  // Initialize flexible array part
  for (size_t i = 0; i < extraSize / sizeof(void*); i++) {
    _Atomic(struct arena_block*)* fieldPtr = (_Atomic(struct arena_block*)*) ((void*) (((char*) data) + desc->objectSize + i * sizeof(void*)));
    atomic_store_explicit(fieldPtr, NULL, memory_order_relaxed);
  }
}

