#include <stdatomic.h>
#include <stddef.h>

#include <flup/concurrency/mutex.h>

#include "gc/gc.h"
#include "memory/arena.h"
#include "helper.h"
#include "heap/heap.h"

void object_helper_write_ref(struct heap* heap, struct alloc_unit* block, size_t offset, struct alloc_unit* newBlock) {
  heap_block_gc(heap);
  _Atomic(struct alloc_unit*)* fieldPtr = (_Atomic(struct alloc_unit*)*) ((void*) (((char*) block->data) + offset));
  gc_need_remark(atomic_exchange(fieldPtr, newBlock));
  heap_unblock_gc(heap);
}

struct root_ref* object_helper_read_ref(struct heap* heap, struct alloc_unit* block, size_t offset) {
  heap_block_gc(heap);
  _Atomic(struct alloc_unit*)* fieldPtr = (_Atomic(struct alloc_unit*)*) ((void*) (((char*) block->data) + offset));
  struct root_ref* new = heap_new_root_ref_unlocked(heap, atomic_load(fieldPtr));
  gc_need_remark(block);
  heap_unblock_gc(heap);
  return new;
}


