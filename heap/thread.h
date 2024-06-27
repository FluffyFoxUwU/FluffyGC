#ifndef UWU_F495C647_28BD_4F8C_8ACD_4E67362B2722_UWU
#define UWU_F495C647_28BD_4F8C_8ACD_4E67362B2722_UWU

#include <pthread.h>

#include <flup/concurrency/mutex.h>
#include <flup/data_structs/list_head.h>

#include "memory/arena.h"

struct thread {
  struct heap* ownerHeap;
  
  flup_list_head rootEntries;
  size_t rootSize;
};

struct thread* thread_new(struct heap* owner);
void thread_free(struct thread* self);

struct root_ref* thread_new_root_ref_no_gc_block(struct thread* self, struct alloc_unit* block);
void thread_unref_root_no_gc_block(struct thread* self, struct root_ref* ref);

// Preallocation
struct root_ref* thread_prealloc_root_ref(struct thread* self);
void thread_new_root_ref_from_prealloc_no_gc_block(struct thread* self, struct root_ref* prealloc, struct alloc_unit* block);

#endif
