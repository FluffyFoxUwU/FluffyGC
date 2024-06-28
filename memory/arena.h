#ifndef UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU
#define UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU

#include <stdatomic.h>
#include <stddef.h>

#include <flup/data_structs/list_head.h>
#include <flup/concurrency/mutex.h>

#include "gc/gc.h"
#include "object/descriptor.h"

struct alloc_tracker {
  atomic_size_t currentUsage;
  // Bytes occupied for purely metadata
  atomic_size_t metadataUsage;
  // Bytes occupied for actual data
  atomic_size_t nonMetadataUsage;
  
  // Number of bytes ever allocated
  atomic_size_t lifetimeBytesAllocated;
  size_t maxSize;
  
  _Atomic(struct alloc_unit*) head;
};

struct alloc_unit {
  // If current->next == NULL then its end of detached head :3
  struct alloc_unit* next;
  size_t size;
  // Atomic so that new object would have NULL value
  // and can be atomically set the corresponding descriptor
  // without blocking GC too long if descriptor so happen
  // to have insanely large amount of fields and it takes
  // to initialize those fields
  _Atomic(struct descriptor*) desc;
  struct gc_block_metadata gcMetadata;
  void* data;
};

// Snapshot of list of heap objects
// at the time of snapshot for GC traversal
struct alloc_tracker_snapshot {
  struct alloc_unit* head;
};

void arena_take_snapshot(struct alloc_tracker* self, struct alloc_tracker_snapshot* snapshot);
void arena_unsnapshot(struct alloc_tracker* self, struct alloc_tracker_snapshot* snapshot, struct alloc_unit* blk);

struct alloc_tracker* arena_new(size_t size);
void arena_free(struct alloc_tracker* self);

struct alloc_unit* arena_alloc(struct alloc_tracker* self, size_t size);
void arena_dealloc(struct alloc_tracker* self, struct alloc_unit* blk);

#endif
