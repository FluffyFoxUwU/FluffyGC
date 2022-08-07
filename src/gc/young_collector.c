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

#include "cardtable_iterator.h"
#include "parallel_heap_iterator.h"
#include "parallel_marker.h"
#include "young_collector.h"
#include "full_collector.h"
#include "gc.h"
#include "common.h"

bool gc_young_pre_collect(struct gc_state* self) {
  return true;
}

static void doPromote(struct gc_state* self, struct region_reference* currentObject, struct object_info* currentObjectInfo, struct region_reference* relocatedLocation) {
  struct object_info* relocatedInfo = &self->heap->oldObjects[relocatedLocation->id];
  assert(relocatedInfo->isValid == false);
  assert(!relocatedInfo->justMoved);
  assert(!relocatedInfo->isMoved);
  assert(!currentObjectInfo->isMoved);
  assert(relocatedLocation);

  *relocatedInfo = *currentObjectInfo;
  relocatedInfo->isValid = true;
  relocatedInfo->justMoved = true;
  relocatedInfo->regionRef = relocatedLocation;
  relocatedInfo->moveData.oldLocation = currentObject;
  relocatedInfo->moveData.oldLocationInfo = currentObjectInfo;
  relocatedInfo->moveData.newLocation = relocatedLocation;
  relocatedInfo->moveData.newLocationInfo = relocatedInfo;

  heap_reset_object_info(self->heap, currentObjectInfo);
  currentObjectInfo->isValid = false;
  currentObjectInfo->isMoved = true;
  currentObjectInfo->regionRef = NULL;
  currentObjectInfo->moveData.oldLocation = currentObject;
  currentObjectInfo->moveData.oldLocationInfo = currentObjectInfo;
  currentObjectInfo->moveData.newLocation = relocatedLocation;
  currentObjectInfo->moveData.newLocationInfo = relocatedInfo;

  // Copy over the data
  region_write(relocatedLocation->owner, relocatedLocation, 0, currentObject->data, currentObject->dataSize);
}

static void fixAddr(struct gc_state* self, bool aboutToCallOldGC) {
  struct heap* heap = self->heap;  
  
  // Fix up addresses in promoted objects
  // No need to clear the object_info as
  // it will be cleared whenever new object
  // created at that location 
  gc_parallel_heap_iterator_do(self, true, ^bool (struct object_info* currentObjectInfo, int idx) {
    if (currentObjectInfo->isMoved) {
      assert(currentObjectInfo->moveData.newLocationInfo);
      assert(currentObjectInfo->moveData.newLocationInfo->isValid);
      assert(currentObjectInfo->moveData.newLocationInfo->justMoved);
      assert(currentObjectInfo->moveData.newLocationInfo->moveData.oldLocationInfo == currentObjectInfo);

      gc_fix_object_refs(self, currentObjectInfo->moveData.newLocationInfo);
    } else {
      if (aboutToCallOldGC && currentObjectInfo->isValid)
        gc_fix_object_refs(self, currentObjectInfo);
    }
    return true;
  }, NULL);

  cardtable_iterator_do(heap, heap->oldObjects, heap->oldToYoungCardTable, heap->oldToYoungCardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    gc_fix_object_refs(self, info);
  });
  
  gc_fix_root(self);
}

void gc_young_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  
  
  // Mark phase
  profiler_begin(self->profiler, "mark");
  gc_parallel_marker(self, true);
  profiler_end(self->profiler);

  // Clear weak refs
  profiler_begin(self->profiler, "clear-weak-refs");
  gc_clear_non_strong_refs(self, true);
  profiler_end(self->profiler);

  // Sweep phase
  profiler_begin(self->profiler, "sweep");
  int lastPos = 0;
  int retryCounts = 0;

  while (lastPos < self->heap->youngGeneration->sizeInCells) {
    bool sucess = gc_parallel_heap_iterator_do(self, true, ^bool (struct object_info* currentObjectInfo, int idx) {
      atomic_store(&currentObjectInfo->strongRefCount, 0);      
      
      struct region_reference* currentObject = currentObjectInfo->regionRef;
      if (!currentObjectInfo->isValid)
        return true;

      if (atomic_exchange(&currentObjectInfo->isMarked, false) == false) {
        heap_sweep_an_object(heap, currentObjectInfo);
        return true;
      }
      
      struct region_reference* relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
      if (!relocatedLocation)
        return false;

      doPromote(self, currentObject, currentObjectInfo, relocatedLocation);
      return true;
    }, &lastPos);

    if (sucess)
      break;
    
    switch (retryCounts) {
      case 0:
        fixAddr(self, true);
        gc_trigger_old_collection(self, REPORT_PROMOTION_FAILURE);
        break;
      case 1:
        gc_trigger_full_collection(self, REPORT_PROMOTION_FAILURE, false);
        break;
    }

    if (retryCounts > 1) {
      puts("failed c");
      heap_report_printf(heap, "Old generation exhausted cannot promote");
      break;
    }

    retryCounts++;
  }
  profiler_end(self->profiler);

  profiler_begin(self->profiler, "fix-refs"); 
  fixAddr(self, false);
  profiler_end(self->profiler); 
  
  profiler_begin(self->profiler, "reset-relocation-info"); 
  gc_clear_old_to_young_card_table(self);
  profiler_end(self->profiler); 

  profiler_begin(self->profiler, "clearing-young-gen"); 
  region_wipe(self->heap->youngGeneration);
  profiler_end(self->profiler); 
}

void gc_young_post_collect(struct gc_state* self) {
}

