#include "heap_free_block_searchers.h"
#include "heap.h"
#include "config.h"
#include "bug.h"

static struct heap_block* bestFit(struct heap* self, struct heap_block* block, size_t size) {
  struct heap_block* smallest = block;
  while (block) {
    if (block->blockSize >= size && block->blockSize < smallest->blockSize)
      smallest = block;
    block = block->next;
  }
  return smallest;
}

static struct heap_block* firstFit(struct heap* self, struct heap_block* block, size_t size) {
  while (block && block->blockSize < size)
    block = block->next;
  return block;
}

struct heap_block* heap_find_free_block(struct heap* self, struct heap_block* block, size_t size) {
  if (IS_ENABLED(CONFIG_HEAP_FREE_BLOCK_BEST_FIT))
    return bestFit(self, block, size);
  else if (IS_ENABLED(CONFIG_HEAP_FREE_BLOCK_FIRST_FIT))
    return firstFit(self, block, size);
  
  // Just incase
  BUG();
}
