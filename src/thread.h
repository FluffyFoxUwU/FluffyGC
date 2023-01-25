#ifndef _headers_1673680059_FluffyGC_thread
#define _headers_1673680059_FluffyGC_thread

#include <stdbool.h>
#include <threads.h>

#include "list.h"
#include "heap_local_heap.h"

struct heap;
struct managed_heap;
struct object;
struct small_object_cache;

enum context_type {
  // This always single thread
  CONTEXT_MAIN_GC_THREAD,
  
  // These may have more than one context
  CONTEXT_USER,
  CONTEXT_SWEEPER_WORKER,
  CONTEXT_MARKER_WORKER,
  CONTEXT_COMPACTER_WORKER,
};

struct context {
  enum context_type contextType;
  
  struct small_object_cache* listNodeCache;
  int blockCount;
 
  list_t* pinnedObjects;
  list_t* root;
  
  struct managed_heap* managedHeap;
  struct heap_local_heap localHeap;
  struct heap* heap;
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

