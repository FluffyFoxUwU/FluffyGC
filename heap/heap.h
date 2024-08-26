#ifndef UWU_A20CD05E_D0C0_425B_B0C9_876974A0CA1B_UWU
#define UWU_A20CD05E_D0C0_425B_B0C9_876974A0CA1B_UWU

#include <stddef.h>

#include <flup/concurrency/mutex.h>
#include <flup/data_structs/list_head.h>
#include <flup/thread/thread_local.h>

#include "heap/generation.h"
#include "memory/alloc_tracker.h"
#include "memory/alloc_context.h"
#include "object/descriptor.h"

struct descriptor;
struct thread;

struct heap {
  struct generation* gen;
  
  flup_thread_local* currentThread;
  flup_mutex* threadListLock;
  flup_list_head threads;
};

struct root_ref {
  flup_list_head node;
  struct alloc_unit* obj;
};

// Thread management
struct thread* heap_get_current_thread(struct heap* self);
void heap_iterate_threads(struct heap* self, void (^iterator)(struct thread* thrd));

struct thread* heap_attach_thread(struct heap* self);
void heap_detach_thread(struct heap* self);

struct heap* heap_new(size_t size);
void heap_free(struct heap* self);

struct root_ref* heap_alloc(struct heap* self, size_t size);
struct root_ref* heap_alloc_with_descriptor(struct heap* self, struct descriptor* desc, size_t extraSize);

struct root_ref* heap_new_root_ref_unlocked(struct heap* self, struct alloc_unit* obj);
void heap_root_unref(struct heap* self, struct root_ref* ref);

// These can't be nested
void heap_block_gc(struct heap* self);
void heap_unblock_gc(struct heap* self);

struct alloc_context* heap_get_alloc_context(struct heap* self);

#endif
