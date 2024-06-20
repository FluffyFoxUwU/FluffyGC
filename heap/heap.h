#ifndef UWU_A20CD05E_D0C0_425B_B0C9_876974A0CA1B_UWU
#define UWU_A20CD05E_D0C0_425B_B0C9_876974A0CA1B_UWU

#include <stddef.h>

#include <flup/concurrency/mutex.h>
#include <flup/data_structs/list_head.h>

#include "heap/generation.h"
#include "memory/arena.h"
#include "object/descriptor.h"

struct descriptor;

struct heap {
  struct generation* gen;
  
  struct thread* mainThread;
};

struct root_ref {
  flup_list_head node;
  struct arena_block* obj;
};

struct heap* heap_new(size_t size);
void heap_free(struct heap* self);

struct root_ref* heap_alloc(struct heap* self, size_t size);
struct root_ref* heap_alloc_with_descriptor(struct heap* self, struct descriptor* desc, size_t extraSize);

struct root_ref* heap_new_root_ref_unlocked(struct heap* self, struct arena_block* obj);
void heap_root_unref(struct heap* self, struct root_ref* ref);

// These can't be nested
void heap_block_gc(struct heap* self);
void heap_unblock_gc(struct heap* self);

#endif
