#ifndef _headers_1673680059_FluffyGC_thread
#define _headers_1673680059_FluffyGC_thread

#include <stdbool.h>
#include <threads.h>

#include "concurrency/completion.h"
#include "concurrency/event.h"
#include "list.h"
#include "memory/heap_local_heap.h"

struct heap;
struct managed_heap;
struct object;
struct small_object_cache;

// Represents user context

struct context {
  struct managed_heap* managedHeap;
  struct heap_local_heap localHeap;
  struct heap* heap;
  
  struct small_object_cache* listNodeCache;
  
  // Amount of context_block_gc calls
  atomic_uint blockCount;
 
  list_t* pinnedObjects;
  list_t* root;
};

struct context* context_new();
void context_free(struct context* self);

extern thread_local struct context* context_current;

// This can be nested
void context_block_gc();
void context_unblock_gc();

bool context_add_pinned_object(struct object* obj);
void context_remove_pinned_object(struct object* obj);

bool context_add_root_object(struct object* obj);
void context_remove_root_object(struct object* obj);

#endif

