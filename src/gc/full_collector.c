#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "../root_iterator.h"
#include "../root.h"
#include "../heap.h"
#include "../config.h"
#include "../region.h"
#include "../descriptor.h"
#include "../profiler.h"

#include "gc/parallel_heap_iterator.h"
#include "heap.h"
#include "region.h"
#include "young_collector.h"
#include "gc.h"
#include "marker.h"

bool gc_full_pre_collect(struct gc_state* self) {
  gc_parallel_heap_iterator_do(self, false, ^bool (struct object_info *info, int idx) {  
    self->fullGC.oldLookup[idx] = NULL;
    return false;
  }, NULL);

  return true;
}

static void fixRefs(struct gc_state* self) {
  struct heap* heap = self->heap;  
  
  gc_fixer_callback fixer = ^void* (void* ptr) {
    if (!ptr)
      return NULL;

    // Not old object
    if (region_get_cellid(heap->oldGeneration, ptr) < 0)
      return ptr;
    
    int id = region_get_cellid(heap->oldGeneration, ptr);
    struct region_reference* newObject = self->fullGC.oldLookup[id];
    assert(newObject);

    return newObject->untypedRawData;
  };
  
  gc_parallel_heap_iterator_do(self, true, ^bool (struct object_info *info, int idx) {
    if (!info->isValid)
      return true;
    
    assert(info->type != OBJECT_TYPE_UNKNOWN);
    gc_fix_object_refs_custom(self, info, fixer);
    return true;  
  }, NULL);
  
  gc_parallel_heap_iterator_do(self, false, ^bool (struct object_info *info, int idx) {
    if (!info->isValid)
      return true;
    
    // Fix move data
    if (info->justMoved) {
      struct object_info* oldInfo = info->moveData.oldLocationInfo;
      assert(oldInfo->isMoved);

      oldInfo->moveData.newLocation = info->regionRef;
      oldInfo->moveData.newLocationInfo = info;
    }

    assert(info->type != OBJECT_TYPE_UNKNOWN);
    gc_fix_object_refs_custom(self, info, fixer);
    return true;  
  }, NULL);
}

static void fixRegistry(struct gc_state* self) {
  struct heap* heap = self->heap;  
 
  // This can't be parallized due line 93 and 97
  for (int i = 0; i < heap->oldGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->oldObjects[i];
    if (!currentObjectInfo->isValid || 
        currentObjectInfo->regionRef->id == i) {
      continue;
    }
    
    int newLocation = currentObjectInfo->regionRef->id;
    
    // In theory it shouldnt overlap anything
    // except itself
    struct object_info oldObjectInfo = heap->oldObjects[newLocation];
    if (oldObjectInfo.isValid && oldObjectInfo.regionRef != currentObjectInfo->regionRef) 
      abort();

    heap->oldObjects[newLocation] = *currentObjectInfo;
    heap_reset_object_info(heap, currentObjectInfo);
    currentObjectInfo->isValid = false;
  }
}

void gc_full_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  
 
  if (self->isExplicit) {
    gc_trigger_young_collection(self, REPORT_FULL_COLLECTION);
    gc_trigger_old_collection(self, REPORT_FULL_COLLECTION);
  }

  // Record previous locations
  profiler_begin(self->profiler, "record-refs");
  gc_parallel_heap_iterator_do(self, false, ^bool (struct object_info *info, int idx) {
    if (!info->isValid)
      return true;
    
    assert(self->fullGC.oldLookup[region_get_cellid(heap->oldGeneration, info->regionRef->untypedRawData)] == NULL ||
           self->fullGC.oldLookup[region_get_cellid(heap->oldGeneration, info->regionRef->untypedRawData)] == info->regionRef);
    self->fullGC.oldLookup[region_get_cellid(heap->oldGeneration, info->regionRef->untypedRawData)] = info->regionRef;
    return true;  
  }, NULL);
  profiler_end(self->profiler);
  
  profiler_begin(self->profiler, "compact");
  region_compact(self->heap->oldGeneration);
  profiler_end(self->profiler);
 
  // Fix the objects registry
  // for old generation
  profiler_begin(self->profiler, "fix-registry");
  fixRegistry(self); 
  profiler_end(self->profiler);

  // Last fix ups
  profiler_begin(self->profiler, "fix-refs");
  fixRefs(self);
  profiler_end(self->profiler);
}

void gc_full_post_collect(struct gc_state* self) {
}

