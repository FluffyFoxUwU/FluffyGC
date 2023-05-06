#ifndef _headers_1683286554_FluffyGC_binary_tree
#define _headers_1683286554_FluffyGC_binary_tree

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "attributes.h"
#include "concurrency/rwlock.h"

// Bushy tree (Actually B-tree but like to think as bushy tree lol)

// TODO: Complete B-Tree and fix soc.h

#define BTREE_ORDER (10)

struct btree_range {
  uintptr_t range[2];
};

struct btree_node {
  struct soc_chunk* chunk;
  void* values[BTREE_ORDER]; 
  struct btree_node* childs[BTREE_ORDER + 1];
};

struct btree {
  struct btree_node* root;
  
};

void btree_init(struct btree* self);
void btree_cleanup(struct btree* self);

int btree_add(struct btree* self, struct btree_node* node);
int btree_remove(struct btree* self, struct btree_node* node);
struct btree_node* btree_find(struct btree* self, uintptr_t search);

#endif

