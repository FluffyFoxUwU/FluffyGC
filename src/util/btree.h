#ifndef _headers_1683286554_FluffyGC_binary_tree
#define _headers_1683286554_FluffyGC_binary_tree

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "attributes.h"
#include "bits.h"
#include "concurrency/rwlock.h"
#include "util/util.h"

// Bushy tree (Actually B-tree but like to think as bushy tree lol)
// Refuses on overlapping ranges

// TODO: Complete B-Tree and fix soc.h

enum btree_node_type {
  BTREE_TYPE_EMPTY,
  BTREE_TYPE_NODE,
  BTREE_TYPE_LEAF // Node only contains pointer to data
};

// This wrapped because lower bits used for metadata
// and should not be accessed directly
struct btree_node;
struct btree_node_leaf;
struct btree_node_ptr {
  union {
    struct btree_node* node;
    void* data;
  };
};

// 2 lower bits reserved
#define BTREE_NODE_RESERVED_BITS 2
#define BTREE_NODE_MIN_ALIGNMENT BIT_ULL(BTREE_NODE_RESERVED_BITS)
#define BTREE_NODE_PTR_MASK (BTREE_NODE_MIN_ALIGNMENT - 1)

#define BTREE_NODE_GET_META(x) ((int) (((uintptr_t) (x.ptr)) & BTREE_NODE_PTR_MASK))
#define BTREE_NODE_GET_PTR(x) ((void*) (((uintptr_t) (x.ptr)) & (~BTREE_NODE_PTR_MASK)))
#define BTREE_NODE_GET_LEAF(x) ((struct btree_node_leaf*) (((uintptr_t) (x.leaf)) & (~BTREE_NODE_PTR_MASK)))
#define BTREE_NODE_GET_INTERNAL(x) ((struct btree_node_internal*) (((uintptr_t) (x.internal)) & (~BTREE_NODE_PTR_MASK)))

struct btree_range {
  uintptr_t low;
  uintptr_t high;
};

#define BTREE_ORDER (sizeof(uintptr_t) == 8 ? 30 : 62)

struct btree_node {
  struct soc_chunk* chunk;
  int pivotsCount;
  
  uintptr_t pivots[BTREE_ORDER];
  struct btree_node_ptr childs[BTREE_ORDER + 1];
  void* paddingBecauseILikeRound512;
};

struct btree {
  struct btree_node_ptr root;
};

// void btree_init(struct btree* self);
// void btree_cleanup(struct btree* self);

// int btree_add_range(struct btree* self, struct btree_range* range, void* value);
// int btree_remove_range(struct btree* self, struct btree_range* range);

// void* btree_find(struct btree* self, uintptr_t search);

#endif

