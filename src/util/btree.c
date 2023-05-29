#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#include "btree.h"
#include "bug.h"
#include "memory/soc.h"
#include "util/util.h"

SOC_DEFINE_ADVANCED(nodesCache, SOC_DEFAULT_CHUNK_SIZE, alignof(struct btree_node), sizeof(struct btree_node))

static bool isOverlapping(struct btree_range* range, uintptr_t val) {
  return range->low <= val && range->high >= val;
}

void btree_init(struct btree* self) {
  self->root = (struct btree_node_ptr) {};
}

void btree_cleanup(struct btree* self) {
  self->root = (struct btree_node_ptr) {};
}

// [2  5  8  9]
// /\ /\ /\ /\
//A  B  C  D  E

static void insertNotFull(struct btree_node* node, struct btree_range* range, struct btree_node_ptr ptr) {
  uintptr_t* insertAtPtr = util_find_smallest_but_larger_or_equal_than(node->pivots, node->pivotsCount, range->low);
  int insertAtOffset = insertAtPtr ? indexof(node->pivots, insertAtPtr) : node->pivotsCount;
  
  BUG_ON(node->childs[insertAtOffset].data != NULL);
}

int btree_add_range(struct btree* self, struct btree_range* range, void* node) {
  
}

int btree_remove_range(struct btree* self, struct btree_range* range) {
  
}
