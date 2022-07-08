#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "../root_iterator.h"
#include "../root.h"
#include "../reference.h"
#include "../heap.h"
#include "../config.h"
#include "../region.h"
#include "../descriptor.h"

#include "young_collector.h"
#include "gc.h"
#include "marker.h"

bool gc_full_pre_collect(struct gc_state* self) {
  return true;
}

void gc_full_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  
 
  gc_trigger_old_collection(self, REPORT_FULL_COLLECTION);
  
  // Record previous locations
  for (int i = 0; i < heap->oldGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->oldObjects[i];
    if (currentObjectInfo->isValid)
      self->fullGC.oldLookup[region_get_cellid(heap->oldGeneration, currentObjectInfo->regionRef->data)] = currentObjectInfo->regionRef;
  }

  region_compact(self->heap->oldGeneration);

  gc_fixer_callback fixer = ^void* (void* ptr) {
    if (!ptr)
      return NULL;

    // Unrecognized leave as it is
    struct region* region = heap_get_region(heap, ptr);
    if (region != heap->oldGeneration)
      return ptr;

    cellid_t id = region_get_cellid(region, ptr);
    struct region_reference* newObject = self->fullGC.oldLookup[id];
    assert(newObject);
    return newObject->data;
  };
  
  // Fix the objects registry
  // for old generation
  for (int i = 0; i < heap->oldGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->oldObjects[i];
    if (currentObjectInfo->isValid && currentObjectInfo->regionRef->id != i) {
      int newLocation = currentObjectInfo->regionRef->id;
      
      // In theory it shouldnt overlap anything
      assert(!heap->oldObjects[newLocation].isValid);

      heap->oldObjects[newLocation] = *currentObjectInfo;
      currentObjectInfo->isValid = false;
    }
  }

  // Fix all references
  for (int i = 0; i < heap->youngGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->youngObjects[i];
    if (!currentObjectInfo->isValid)
      continue;
    
    assert(currentObjectInfo->type != OBJECT_TYPE_UNKNOWN);
    gc_fix_object_refs_custom(self, heap->youngGeneration, heap->youngGeneration, currentObjectInfo->regionRef, fixer);
  }
  
  for (int i = 0; i < heap->oldGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->oldObjects[i];
    if (!currentObjectInfo->isValid)
      continue;

    assert(currentObjectInfo->type != OBJECT_TYPE_UNKNOWN);
    gc_fix_object_refs_custom(self, heap->oldGeneration, heap->oldGeneration, currentObjectInfo->regionRef, fixer);
  }
}

void gc_full_post_collect(struct gc_state* self) {
}

