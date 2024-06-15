#include <stdatomic.h>
#include <stddef.h>

#include "gc/gc.h"
#include "memory/arena.h"
#include "helper.h"
#include "heap/heap.h"

void object_helper_write_ref(struct heap* heap, struct arena_block* block, size_t offset, struct root_ref* ref) {
  heap_block_gc(heap);
  _Atomic(struct arena_block*)* fieldPtr = (_Atomic(struct arena_block*)*) ((void*) (((char*) block->data) + offset));
  struct arena_block* oldRef = atomic_exchange(fieldPtr, ref->obj);
  
  gc_on_reference_lost(oldRef);
  heap_unblock_gc(heap);
}

struct root_ref* object_helper_read_ref(struct heap* heap, struct arena_block* block, size_t offset) {
  _Atomic(struct arena_block*)* fieldPtr = (_Atomic(struct arena_block*)*) ((void*) (((char*) block->data) + offset));
  return heap_new_root_ref(heap, atomic_load(fieldPtr));
}


