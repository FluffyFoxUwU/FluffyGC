#include <stdatomic.h>
#include <stdint.h>

#include "btree.h"

void btree_init(struct btree* self) {
  self->root = NULL;
}

void btree_cleanup(struct btree* self) {
  self->root = NULL;
}

int btree_add(struct btree* self, struct btree_node* node) {
  
}

int btree_remove(struct btree* self, struct btree_node* node) {
  
}

struct btree_node* btree_find(struct btree* self, uintptr_t search) {
  struct btree_node* result = NULL;
  struct btree_node* current = self->root;
  
  
  return result;
}
