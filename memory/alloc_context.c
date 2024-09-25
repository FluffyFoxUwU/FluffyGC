#include <mimalloc.h>
#include <stdlib.h>

#include <flup/concurrency/mutex.h>

#include "alloc_tracker.h"
#include "alloc_context.h"

struct alloc_context* alloc_context_new(mi_arena_id_t arena) {
  struct alloc_context* ctx = malloc(sizeof(*ctx));
  if (!ctx)
    return NULL;
  
  *ctx = (struct alloc_context) {};
  if (!(ctx->mimallocHeap = mi_heap_new_in_arena(arena))) {
    free(ctx);
    return NULL;
  }
  return ctx;
}

void alloc_context_add_block(struct alloc_context* self, struct alloc_unit* block) {
  // The "block" is the first make it become the head and tail
  if (!self->allocListHead)
    self->allocListHead = block;
  
  if (self->allocListTail)
    self->allocListTail->next = block;
  self->allocListTail = block;
}

void alloc_context_free(struct alloc_tracker* self, struct alloc_context* ctx) {
  if (!self)
    return;
  
  struct alloc_unit* next = ctx->allocListHead;
  while (next) {
    struct alloc_unit* current = next;
    next = next->next;
    
    alloc_tracker_add_block_to_global_list(self, current);
  }
  
  free(ctx);
}


