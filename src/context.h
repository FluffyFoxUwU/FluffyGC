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

enum context_state {
  CONTEXT_INACTIVE = 0,
  CONTEXT_ACTIVE
};
#define CONTEXT_STATE_COUNT 2

struct context {
  struct list_head list;
  
  struct managed_heap* managedHeap;
  
  // Amount of context_block_gc calls
  unsigned int blockCount;
 
  struct list_head root;
  bool gcInitHookCalled;
  enum context_state state;
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
  struct soc_chunk* chunk;
};

void context_add_pinned_object(struct root_ref* obj);
void context_remove_pinned_object(struct root_ref* obj);

struct root_ref* context_add_root_object(struct object* obj);
int context_remove_root_object(struct root_ref* obj);

#define context_block_gc() do { \
  context__block_gc(); \
  atomic_thread_fence(memory_order_acquire); \
} while (0)

#define context_unblock_gc() do { \
  atomic_thread_fence(memory_order_release); \
  context__unblock_gc(); \
} while (0)

#endif

