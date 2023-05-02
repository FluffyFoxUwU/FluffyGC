#include "heap_free_block_searchers.h"
#include "heap.h"
#include "config.h"
#include "bug.h"
#include "util/list_head.h"

static struct heap_block* bestFit(struct heap* self,  size_t size) {
  struct heap_block* smallest = list_first_entry(&self->recentFreeBlocks, struct heap_block, node);
  struct list_head* current;
  list_for_each(current, &smallest->node) {
    struct heap_block* block = list_entry(current, struct heap_block, node);
    if (block->blockSize >= size && block->blockSize < smallest->blockSize)
      smallest = block;
  }
  
  // Heap too fragmented can't find fit space
  if (smallest->blockSize < size)
    return NULL;
  return smallest;
}

static struct heap_block* firstFit(struct heap* self, size_t size) {
  struct list_head* current;
  list_for_each(current, &list_first_entry(&self->recentFreeBlocks, struct heap_block, node)->node) {
    struct heap_block* block = list_entry(current, struct heap_block, node);
    if (block->blockSize >= size)
      return block;
  }
  
  return NULL;
}

struct heap_block* heap_find_free_block(struct heap* self, size_t size) {
  if (IS_ENABLED(CONFIG_HEAP_FREE_BLOCK_BEST_FIT))
    return bestFit(self, size);
  else if (IS_ENABLED(CONFIG_HEAP_FREE_BLOCK_FIRST_FIT))
    return firstFit(self, size);
  
  // Just incase
  BUG();
}
