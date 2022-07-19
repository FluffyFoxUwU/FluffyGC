#ifndef header_1655956811_allocator_h
#define header_1655956811_allocator_h

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/*
Region structure

ASAN off:
----------------------------\  \-----
| C |   H   | F | C | F | F |\  \ F |
------------------------------\  \---

ASAN on:
----------------------------------------------------\  \-----
| P | C | P |   P   |   H   |   P   | P | C | P | F |\  \ F |
------------------------------------------------------\  \---

Legend:
 C = An object smaller than a cellid
 P = ASAN poisoned region (can he merged)
 H = An object larger than a cellid (merged to one)
 F = Free cellid
*/

struct region {
  void* region;
  
  // Point to last valid cell addresa
  void* topRegion;

  size_t sizeInCells;

  volatile atomic_size_t usage;
  volatile atomic_int bumpPointer;

  // True if used and false if free
  bool* regionUsageLookup;
  struct region_reference** referenceLookup;
  int* cellIDMapping;

  // read lock if its unsafe to compact / wipe
  // write lock if attempting to compact / wipe
  pthread_rwlock_t compactionAndWipingLock;
};

struct region_reference {
  struct region* owner;
  int id;
  void* volatile data;

  size_t dataSize;
  size_t sizeInCells;
  size_t redzoneOffset;
  size_t poisonSize;
};

// Low level region management
struct region* region_new(size_t size);

// Allocation and deallocation routines
// (does not trigger compaction)
struct region_reference* region_alloc(struct region* self, size_t size);
void region_dealloc(struct region* self, struct region_reference* data);

// When bump allocator not working
// try find holes
struct region_reference* region_alloc_or_fit(struct region* self, size_t size);

void region_compact(struct region* self);
void region_wipe(struct region* self);
void region_move_bump_pointer_to_last(struct region* self);

int region_get_cellid(struct region* self, void* data);

// Include requirement that need `size` to be
// multiple of cell size and redzones if ASAN
// enabled
size_t region_get_actual_allocated_size(size_t size);

void region_read(struct region* self, struct region_reference* data, size_t offset, void* buffer, size_t size);
void region_write(struct region* self, struct region_reference* data, size_t offset, void* buffer, size_t size);
struct region_reference* region_get_ref(struct region* self, void* data);

void region_free(struct region* self);
 
#endif




