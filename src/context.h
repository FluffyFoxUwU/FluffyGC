#ifndef _headers_1673680059_FluffyGC_thread
#define _headers_1673680059_FluffyGC_thread

#include <stdbool.h>
#include <stdatomic.h>
#include <threads.h>

#include "concurrency/completion.h"
#include "concurrency/event.h"
#include "util/list_head.h"
#include "util/refcount.h"

struct heap;
struct managed_heap;
struct object;
struct small_object_cache;

// Represents any context

struct context {
  struct managed_heap* managedHeap;
  struct small_object_cache* listNodeCache;
  
  // Amount of context_block_gc calls
  atomic_uint blockCount;
 
  struct list_head root;
};

struct context* context_new();
void context_free(struct context* self);

extern thread_local struct context* context_current;

// This can be nested
void context__block_gc();
void context__unblock_gc();

struct root_ref {
  struct list_head node;
  struct refcount pinCounter;
  _Atomic(struct object*) obj;
};

void context_add_pinned_object(struct root_ref* obj);
void context_remove_pinned_object(struct root_ref* obj);

struct root_ref* context_add_root_object(struct object* obj);
int context_remove_root_object(struct root_ref* obj);

#define context_block_gc() do { \
  atomic_thread_fence(memory_order_acquire); \
  context__block_gc(); \
} while (0)

#define context_unblock_gc() do { \
  context__unblock_gc(); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#endif

