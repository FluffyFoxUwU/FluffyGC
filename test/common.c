#include "common.h"
#include "heap/heap.h"

void fgc_block_gc(fgc_heap* self) {
  heap_block_gc((struct heap*) self);
}

void fgc_unblock_gc(fgc_heap* self) {
  heap_unblock_gc((struct heap*) self);
}
