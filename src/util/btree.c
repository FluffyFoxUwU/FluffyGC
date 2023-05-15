#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include "btree.h"
#include "bug.h"
#include "memory/soc.h"

SOC_DEFINE_ADVANCED(nodesCache, SOC_DEFAULT_CHUNK_SIZE, MAX(alignof(struct btree_node_leaf), alignof(struct btree_node_internal)), MAX(sizeof(struct btree_node_leaf), sizeof(struct btree_node_internal)))

static bool isOverlapping(struct btree_range* range, uintptr_t val) {
  return range->range[0] <= val && range->range[1] >= val;
}

// Works for non-overlapping ranges
static int compareRange(const void* _a, const void* _b) {
  const struct btree_range* a = _a;
  const struct btree_range* b = _b;
}

// parentPtr return what the location of the leaf of interest would be
static struct btree_node_ptr find(struct btree* self, uintptr_t search, struct btree_node_ptr* parentPtr) {
  struct btree_node_ptr result = {};
  struct btree_node_ptr parent = {};
  struct btree_node_ptr current = self->root;
  
  while (BTREE_NODE_GET_PTR(current) != NULL && BTREE_NODE_GET_META(current) == BTREE_TYPE_INTERNAL) {
    struct btree_node_internal* node = BTREE_NODE_GET_INTERNAL(current);
    int i;
    for (i = 0; i < node->keysCount && search >= node->keys[i]; i++)
      ;
    parent = current;
    current = node->childs[i];
  }
  
  if (BTREE_NODE_GET_PTR(current) == NULL)
    goto not_found;
  
  // `current` should contain leaf here
  BUG_ON(BTREE_NODE_GET_META(current) != BTREE_TYPE_LEAF);
  if (isOverlapping(&((struct btree_node_leaf*) BTREE_NODE_GET_PTR(current))->range, search))
    result = current;
not_found:
  if (parentPtr)
    *parentPtr = parent;
  return result;
}

void btree_init(struct btree* self) {
  self->root = (struct btree_node_ptr) {};
}

void btree_cleanup(struct btree* self) {
  self->root = (struct btree_node_ptr) {};
}

int btree_add_range(struct btree* self, struct btree_range* range, void* node) {
  
}

int btree_remove_range(struct btree* self, struct btree_range* range) {
  
}
