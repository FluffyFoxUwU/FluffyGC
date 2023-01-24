#ifndef _headers_1673680059_FluffyGC_thread
#define _headers_1673680059_FluffyGC_thread

#include <stdbool.h>
#include <threads.h>

#include "list.h"
#include "heap_local_heap.h"

struct object;
struct small_object_cache;

struct thread {
  struct small_object_cache* listNodeCache;
  int blockCount;
 
  list_t* pinnedObjects;
  list_t* root;
  
  struct heap_local_heap localHeap;
  struct heap* heap;
};

struct thread* thread_new();
void thread_free(struct thread* self);

extern thread_local struct thread* thread_current;

// This can be nested
void thread_block_gc();
void thread_unblock_gc();

bool thread_add_pinned_object(struct object* obj);
void thread_remove_pinned_object(struct object* obj);

bool thread_add_root_object(struct object* obj);
void thread_remove_root_object(struct object* obj);

#endif

