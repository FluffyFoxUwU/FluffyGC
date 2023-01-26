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
  CONTEXT_MAIN_GC,
  
  // These may have more than one context
  CONTEXT_USER,
  CONTEXT_SWEEPER,
  CONTEXT_MARKER,
  CONTEXT_COMPACTER,
};

/*
Notes:
1. Do not hold any locks when calling another context
2. Don't assume contexts run on seperate thread. this decission aiming
   to have clear boundry between contexts to allow zero use of threadings
3. At any moment there only one context type running
4. Contexts shall not hold lock when its about to wait or perform blocking
   operation (e.g. waiting on other context)

Valid context calls (also graph for blocking hierarchy)
User (1 or more)
 |
 -> Main GC (always 1)
     |
     -> Marker (1 or more)
     -> Sweeper (1 or more)
     -> Compacter (1 or more)

User context:
  Where mutator lives. Shall be blocked on certain functions
  which cause issues if used concurrently while GC running
Main GC context:
  This where most GC operation happens if not on user context
Marker, Sweeper, Compacter GC context:
  Where execution for corresponding action be done
*/

struct context {
  enum context_type contextType;
  
  struct small_object_cache* listNodeCache;
  
  // Amount of context_block_gc calls
  int blockCount;
 
  list_t* pinnedObjects;
  list_t* root;
  
  struct managed_heap* managedHeap;
  struct heap_local_heap localHeap;
  struct heap* heap;
};

struct context* context_new();
void context_free(struct context* self);

// Call this context
void context_call(struct context* self);

extern thread_local struct context* context_current;

// This can be nested
// BUGs when being called from non user context
void context_block_gc();
void context_unblock_gc();

bool context_add_pinned_object(struct object* obj);
void context_remove_pinned_object(struct object* obj);

bool context_add_root_object(struct object* obj);
void context_remove_root_object(struct object* obj);

#endif

