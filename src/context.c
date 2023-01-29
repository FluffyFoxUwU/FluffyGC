#include <stdbool.h>
#include <stdlib.h>
#include <threads.h>
#include <pthread.h>

#include "completion.h"
#include "event.h"
#include "list.h"
#include "soc.h"
#include "context.h"
#include "bug.h"
#include "util.h"

thread_local struct context* context_current = NULL;

struct context* context_new(enum context_type type, void (*onCall)()) {
  struct context* self = malloc(sizeof(*self)); 
  if (!self)
    return NULL;
  *self = (struct context) {
    .type = type,
    .onCallFunc = onCall
  };
  
  if (event_init(&self->onCallEvent) < 0 || completion_init(&self->callDone) < 0)
    goto failure;
  
  if (type != CONTEXT_USER)
    return self;
  
  self->listNodeCache = soc_new(sizeof(list_node_t), 0);
  if (!self->listNodeCache)
    goto failure;
  
  self->pinnedObjects = list_new();
  self->root = list_new();
  if (!self->pinnedObjects || !self->root)
    goto failure;
  
  return self;
  
failure:
  context_free(self);
  return NULL;
}

void context_free(struct context* self) {
  event_cleanup(&self->onCallEvent);
  completion_cleanup(&self->callDone);
  list_destroy2(self->root);
  list_destroy2(self->pinnedObjects);
  soc_free(self->listNodeCache);
  free(self);
}

void context_block_gc() {
  context_current->blockCount++;
  BUG_ON(context_current->type != CONTEXT_USER);
}

void context_unblock_gc() {
  context_current->blockCount--;
  BUG_ON(context_current->type != CONTEXT_USER);
}

static list_node_t* allocListNodeWithContent(void* data) {
  list_node_t* tmp = soc_alloc(context_current->listNodeCache);
  *tmp = (list_node_t) {
    .val = data
  };
  return tmp;
}

bool context_add_pinned_object(struct object* obj) {
  context_block_gc();
  bool res = true;
  list_node_t* node = allocListNodeWithContent(obj);
  if (!node) {
    res = false;
    goto node_alloc_failure;
  }
  
  list_rpush(context_current->pinnedObjects, node);
node_alloc_failure:
  context_unblock_gc();
  return res;
}

void context_remove_pinned_object(struct object* obj) {
  context_block_gc();
  // Search backward as it more likely caller removing recently
  // added object. In long run minimizing time spent searching
  // object to be removed UwU
  list_node_t* node = context_current->pinnedObjects->tail;
  while (node && (node = node->prev)) {
    if (node->val == obj) {
      list_remove2(context_current->pinnedObjects, node);
      soc_dealloc(context_current->listNodeCache, node);
      break;
    }
  }
  
  BUG_ON(!node);
  context_unblock_gc();
}

bool context_add_root_object(struct object* obj) {
  context_block_gc();
  bool res = true;
  list_node_t* node = allocListNodeWithContent(obj);
  if (!node) {
    res = false;
    goto node_alloc_failure;
  }
  
  list_rpush(context_current->root, node);
node_alloc_failure:
  context_unblock_gc();
  return res;
}

void context_remove_root_object(struct object* obj) {
  context_block_gc();
  // Search backward as it more likely caller removing recently
  // added object. In long run minimizing time spent searching
  // object to be removed UwU
  list_node_t* node = context_current->root->tail;
  while (node && (node = node->prev)) {
    if (node->val == obj) {
      list_remove2(context_current->root, node);
      soc_dealloc(context_current->listNodeCache, node);
      break;
    }
  }
  
  BUG_ON(!node);
  context_unblock_gc();
}

