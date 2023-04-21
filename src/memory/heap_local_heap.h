#ifndef _headers_1674460695_FluffyGC_heap_local_heap
#define _headers_1674460695_FluffyGC_heap_local_heap

#include <stdint.h>

struct heap;
struct heap_local_heap;

struct heap_local_heap {
  uintptr_t bumpPointer;
  uintptr_t endLocalHeap;
};

#endif

