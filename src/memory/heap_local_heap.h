#ifndef _headers_1674460695_FluffyGC_heap_local_heap
#define _headers_1674460695_FluffyGC_heap_local_heap

#include <stdint.h>
#include <stddef.h>

struct heap;
struct heap_local_heap;

struct heap_local_heap {
  uintptr_t bumpPointer;
  uintptr_t endLocalHeap;
};

static inline size_t local_heap_get_size(struct heap_local_heap* self) {
  return self->endLocalHeap - self->bumpPointer;
}

#endif

