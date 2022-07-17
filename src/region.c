#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "region.h"
#include "config.h"
#include "asan_stuff.h"
#include "util.h"

static void* get_cell_addr(struct region* self, cellid_t cellid) {
  assert(cellid < self->sizeInCells);
  return self->region + (FLUFFYGC_REGION_CELL_SIZE * cellid);
}

cellid_t region_get_cellid(struct region* self, void* data) {
  if (data > self->topRegion || data < self->region)
    return -1;
  return (data - self->region) / FLUFFYGC_REGION_CELL_SIZE;
}

/*
cellid_t region_get_cellid_mapped(struct region* self, void* data) {
  cellid_t id = region_get_cellid(self, data);
  if (id < 0)
    return id;

  id = self->cellIDMapping[id];
  return id;
}
*/

struct region_reference* region_get_ref(struct region* self, void* data) {
  if (data < self->region || data > self->topRegion)
    return NULL;
  cellid_t id = region_get_cellid(self, data);
  if (id < 0)
    return NULL;

  id = self->cellIDMapping[id];

  struct region_reference* data2 = self->referenceLookup[id];
  return data2;
}

size_t region_get_actual_allocated_size(size_t size) {
  size_t requiredSize = size;
  if (FLUFFYGC_ASAN_ENABLED && FLUFFYGC_REGION_ENABLE_POISONING) {
    // Size has to be multiples of poison
    // granularity for poisoning
    if (requiredSize % FLUFFYGC_REGION_POISON_GRANULARITY > 0)
      requiredSize += FLUFFYGC_REGION_POISON_GRANULARITY - (requiredSize % FLUFFYGC_REGION_POISON_GRANULARITY);

    requiredSize *= 3;
  }
  
  cellid_t sizeInCells = requiredSize / FLUFFYGC_REGION_CELL_SIZE;
  if (requiredSize % FLUFFYGC_REGION_CELL_SIZE > 0)
    sizeInCells++;
  
  return sizeInCells * FLUFFYGC_REGION_CELL_SIZE;
}

static void poison_cells(struct region* self, cellid_t cellStart, int count) {
  assert(cellStart + count < self->sizeInCells); /* Check cells range */
  if (!FLUFFYGC_ASAN_ENABLED || !FLUFFYGC_REGION_ENABLE_POISONING)
    return;

  for (unsigned int i = 0; i < count; i++)
    asan_poison_memory_region(get_cell_addr(self, cellStart + i), FLUFFYGC_REGION_CELL_SIZE);
}

static void unpoison_cells(struct region* self, cellid_t cellStart, int count) {
  assert(cellStart + count < self->sizeInCells); /* Check cells range */
  if (!FLUFFYGC_ASAN_ENABLED || !FLUFFYGC_REGION_ENABLE_POISONING)
    return;

  for (unsigned int i = 0; i < count; i++)
    asan_unpoison_memory_region(get_cell_addr(self, cellStart + i), FLUFFYGC_REGION_CELL_SIZE);
}

struct region* region_new(size_t size) {
  struct region* self = malloc(sizeof(struct region));
  if (!self)
    return NULL;

  // Correct the size
  size_t correctedSize = size;
  if (correctedSize % FLUFFYGC_REGION_CELL_SIZE != 0) {
    correctedSize -= correctedSize % FLUFFYGC_REGION_CELL_SIZE;
    correctedSize += FLUFFYGC_REGION_CELL_SIZE;
  }

  // Limits to 2 billions cells
  // because some calculation uses
  // int (which may overflow when used 
  // unsigned int) and couldnt find 
  // them all
  if (correctedSize >= INT_MAX)
    abort();

  atomic_init(&self->bumpPointer, 0);
  atomic_init(&self->usage, 0);
  pthread_rwlock_init(&self->compactionAndWipingLock, NULL);
 
  size_t sizeInCells = correctedSize / FLUFFYGC_REGION_CELL_SIZE;

  // Allocation calls
  self->region = aligned_alloc(FLUFFYGC_REGION_CELL_ALIGNMENT, correctedSize);
  self->referenceLookup = calloc(sizeInCells, sizeof(struct region_reference*));
  self->regionUsageLookup = calloc(sizeInCells, sizeof(*self->regionUsageLookup));
  self->cellIDMapping = calloc(sizeInCells, sizeof(*self->cellIDMapping));
  
  self->sizeInCells = sizeInCells;
  self->topRegion = self->region + sizeInCells * FLUFFYGC_REGION_CELL_SIZE;
 
  // Check in bulk if the allocation call is
  // just literally allocates memory (malloc, 
  // calloc, aligned_alloc, etc)
  if (!self->region ||
      !self->referenceLookup ||
      !self->regionUsageLookup ||
      !self->cellIDMapping)
    goto failure;
  
  for (cellid_t i = 0; i < sizeInCells; i++)
    self->cellIDMapping[i] = i;

  poison_cells(self, 0, sizeInCells - 1);
  return self;

  failure:
  region_free(self);
  return NULL;
}

void region_free(struct region* self) {
  if (!self)
    return;

  if (self->region) {
    if (self->referenceLookup && self->regionUsageLookup)
      region_wipe(self);
    unpoison_cells(self, 0, self->sizeInCells - 1);
  }
  pthread_rwlock_destroy(&self->compactionAndWipingLock);

  free(self->cellIDMapping);
  free(self->regionUsageLookup);
  free(self->referenceLookup);
  free(self->region);
  free(self);
}

static void region_make_unavailable(struct region* self, cellid_t cell, int count) {
  for (int i = 0; i < count; i++) {
    assert(self->regionUsageLookup[cell + i] == true);
    self->regionUsageLookup[cell + i] = false;
  }

  poison_cells(self, cell, count);
}

static void region_make_available_no_unpoison(struct region* self, cellid_t cell, int count) {
  for (int i = 0; i < count; i++) {
    assert(self->regionUsageLookup[cell + i] == false);
    self->regionUsageLookup[cell + i] = true;
  }
}

struct region_reference* region_alloc(struct region* self, size_t dataSize) {
  if (dataSize == 0)
    return NULL;

  // For left and right redzones and actual data in the middle
  size_t redzoneOffset = 0;
  size_t requiredSize = dataSize;
  size_t poisonSize = 0;
  if (FLUFFYGC_ASAN_ENABLED && FLUFFYGC_REGION_ENABLE_POISONING) {
    // Size has to be multiples of poison
    // granularity for poisoning
    if (requiredSize % FLUFFYGC_REGION_POISON_GRANULARITY > 0)
      requiredSize += FLUFFYGC_REGION_POISON_GRANULARITY - (requiredSize % FLUFFYGC_REGION_POISON_GRANULARITY);

    redzoneOffset = requiredSize;
    poisonSize = requiredSize;
    requiredSize *= 3;
  }
  
  cellid_t sizeInCells = requiredSize / FLUFFYGC_REGION_CELL_SIZE;
  if (requiredSize % FLUFFYGC_REGION_CELL_SIZE > 0)
    sizeInCells++;

  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  
  cellid_t newCellID;
  if (!util_atomic_add_if_less_int(&self->bumpPointer, sizeInCells, self->sizeInCells, &newCellID))
    goto failure;

  struct region_reference* ref = malloc(sizeof(*ref));
  if (!ref) {
    atomic_fetch_sub(&self->bumpPointer, sizeInCells);
    goto failure;
  }

  atomic_fetch_add(&self->usage, sizeInCells * FLUFFYGC_REGION_CELL_SIZE);

  ref->id = newCellID;
  ref->data = get_cell_addr(self, newCellID) + redzoneOffset;
  ref->sizeInCells = sizeInCells;
  ref->dataSize = dataSize;
  ref->redzoneOffset = redzoneOffset;
  ref->poisonSize = poisonSize;
  ref->isPinned = false;
  ref->owner = self;

  self->referenceLookup[newCellID] = ref;
  self->cellIDMapping[region_get_cellid(self, ref->data)] = newCellID;

  region_make_available_no_unpoison(self, ref->id, sizeInCells);
  if (FLUFFYGC_REGION_ENABLE_POISONING)
    asan_unpoison_memory_region(ref->data, poisonSize);

  pthread_rwlock_unlock(&self->compactionAndWipingLock);
  return ref;

  failure:
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
  return NULL;
}

struct region_reference* region_alloc_or_fit(struct region* self, size_t size) {
  return region_alloc(self, size);
}

static void region_dealloc_no_lock(struct region* self, struct region_reference* ref) {
  if (!ref)
    return;
  
  atomic_fetch_sub(&self->usage, ref->sizeInCells * FLUFFYGC_REGION_CELL_SIZE);

  region_make_unavailable(self, ref->id, ref->sizeInCells);
  self->referenceLookup[ref->id] = NULL;
  cellid_t mappingIndex = region_get_cellid(self, ref->data);
  self->cellIDMapping[mappingIndex] = mappingIndex;
  free(ref);
}

void region_dealloc(struct region* self, struct region_reference* ref) {
  if (!ref)
    return;
  
  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  region_dealloc_no_lock(self, ref);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}

void region_wipe(struct region *self) {
  pthread_rwlock_wrlock(&self->compactionAndWipingLock);
  for (int i = 0; i < self->sizeInCells; i++)
    region_dealloc_no_lock(self, self->referenceLookup[i]);

  atomic_store(&self->bumpPointer, 0);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}

static bool region_move_cell(struct region* self, cellid_t src, cellid_t dest) {
  assert(src < self->sizeInCells);
  assert(dest < self->sizeInCells);
  assert(self->regionUsageLookup[src] == true);
  assert(self->regionUsageLookup[dest] == false);
  
  struct region_reference* ref = self->referenceLookup[src];
  if (ref->isPinned > 0)
    return false; /* Object is pinned can't move */

  void* newCellArea = get_cell_addr(self, dest);
  void* oldArea = ref->data;
  ref->data = newCellArea + ref->redzoneOffset;
  ref->id = dest;
  
  region_make_available_no_unpoison(self, dest, ref->sizeInCells);
  if (FLUFFYGC_REGION_ENABLE_POISONING)
    asan_unpoison_memory_region(ref->data, ref->poisonSize);
  
  // Copy the data
  memcpy(ref->data, oldArea, ref->dataSize);

  region_make_unavailable(self, src, ref->sizeInCells);
   
  self->referenceLookup[src] = NULL;
  self->referenceLookup[dest] = ref;

  cellid_t newMapping = region_get_cellid(self, ref->data);
  cellid_t oldMapping = region_get_cellid(self, oldArea);
  self->cellIDMapping[newMapping] = ref->id;
  self->cellIDMapping[oldMapping] = oldMapping;

  return true;
}

static void region_move_bump_pointer_to_last_no_lock(struct region* self) {
  cellid_t current = atomic_load(&self->bumpPointer);
  while (current >= 0 && !self->regionUsageLookup[current])
    current--;

  if (current < 0 || self->regionUsageLookup[current])
    current++;

  atomic_store(&self->bumpPointer, current);
}

void region_compact(struct region* self) {
  pthread_rwlock_wrlock(&self->compactionAndWipingLock);
  cellid_t dest = 0;
  cellid_t src = 0;

  // Anything after bumpPointer is just free cells
  while (src < self->bumpPointer) {
    if (self->regionUsageLookup[src]) {
      struct region_reference* ref = self->referenceLookup[src];
      
      // Check if destination is overlapped
      // with pinned or used object if yes
      // skip over
      for (cellid_t i = dest; i < dest + ref->sizeInCells; i++) {
        struct region_reference* current = self->referenceLookup[i];
        if (current && current->isPinned > 0) {
          dest = i + current->sizeInCells;
          break;
        }
      }
      
      // Skip pinned objects
      if (ref->isPinned) {
        src += ref->sizeInCells;
        continue;
      }

      assert(dest < self->sizeInCells);
      assert(src < self->sizeInCells);

      bool overlapping = false;
      for (cellid_t i = dest; i < dest + ref->sizeInCells; i++) {
        if (self->regionUsageLookup[i]) {
          overlapping = true;
          break;
        }
      }
      
      if (!overlapping && src != dest)
        region_move_cell(self, src, dest);

      src += ref->sizeInCells;
      dest += ref->sizeInCells;
    } else {
      src += 1;
    }
  }

  region_move_bump_pointer_to_last_no_lock(self);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}

void region_read(struct region* self, struct region_reference* ref, size_t offset, void* buffer, size_t size) {
  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  memcpy(buffer, ref->data + offset, size);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}

void region_write(struct region* self, struct region_reference* ref, size_t offset, void* buffer, size_t size) {
  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  memcpy(ref->data + offset, buffer, size);
  pthread_rwlock_unlock(&self->compactionAndWipingLock); 
}

void region_pin_object(struct region* self, struct region_reference* data) {
  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  atomic_fetch_add(&data->isPinned, 1);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}

void region_unpin_object(struct region* self, struct region_reference* data) {
  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  int prev = atomic_fetch_sub(&data->isPinned, 1);
  assert(prev > 0);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}

void region_move_bump_pointer_to_last(struct region* self) {
  pthread_rwlock_rdlock(&self->compactionAndWipingLock);
  region_move_bump_pointer_to_last_no_lock(self);
  pthread_rwlock_unlock(&self->compactionAndWipingLock);
}






