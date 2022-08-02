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
#include "young_collector.h"
#include "full_collector.h"
#include "gc.h"
#include "marker.h"

bool gc_young_pre_collect(struct gc_state* self) {
  return true;
}

static void markPhase(struct gc_state* self) {
  profiler_begin(self->profiler, "mark");
  
  struct heap* heap = self->heap;  
  struct gc_marker_args defaultArgs = gc_marker_builder()
            ->gc_state(self)
            ->ignore_weak(true)
            ->only_in(heap->youngGeneration)
            ->build();

  // Marking phase
  root_iterator_run(root_iterator_builder()
      ->ignore_weak(true)
      ->heap(heap)
      ->only_in(heap->youngGeneration)
      ->consumer(^void (struct root_reference* ref, struct object_info* info) {
        atomic_fetch_add(&info->strongRefCount, 1);
        gc_marker_mark(gc_marker_builder()
            ->copy_from(defaultArgs)
            ->object_info(info)
            ->build()
        );
      })
      ->build()
  ); 

  cardtable_iterator_do(heap, heap->oldObjects, heap->oldToYoungCardTable, heap->oldToYoungCardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    gc_marker_mark(gc_marker_builder()
        ->copy_from(defaultArgs)
        ->object_info(info)
        ->build()
    );
  });
  
  profiler_end(self->profiler);
}

static void clearWeakRefs(struct gc_state* self) {
  profiler_begin(self->profiler, "clear-weak-refs");
  
  for (int i = 0; i < self->heap->youngGeneration->sizeInCells; i++) {
    if (!self->heap->youngObjects[i].isValid)
      continue;
    
    if (self->isExplicit)
      gc_clear_soft_refs(self, &self->heap->youngObjects[i]);
    gc_clear_weak_refs(self, &self->heap->youngObjects[i]);
  }
  
  root_iterator_run(root_iterator_builder()
      ->ignore_weak(true)
      ->heap(self->heap)
      ->only_in(self->heap->youngGeneration)
      ->consumer(^void (struct root_reference* ref, struct object_info* info) {
        if (info->strongRefCount == 0)
          ref->data = NULL;
      })
      ->build()
  ); 
  
  cardtable_iterator_do(self->heap, self->heap->oldObjects, self->heap->oldToYoungCardTable, self->heap->oldToYoungCardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    if (!info->isValid)
      return;

    if (self->isExplicit)
      gc_clear_soft_refs(self, info);
    gc_clear_weak_refs(self, info);
  });

  profiler_end(self->profiler);
}

static void resetRefCounts(struct gc_state* self) {
  profiler_begin(self->profiler, "reset-ref-counts");
  for (int i = 0; i < self->heap->youngGeneration->sizeInCells; i++)
    atomic_store(&self->heap->youngObjects[i].strongRefCount, 0);
    
  profiler_end(self->profiler);
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
  for (int i = 0; i < heap->youngGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->youngObjects[i];

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
  }  

  cardtable_iterator_do(heap, heap->oldObjects, heap->oldToYoungCardTable, heap->oldToYoungCardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    gc_fix_object_refs(self, info);
  });
  
  gc_fix_root(self);
}

void gc_young_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  
  
  // Mark phase
  markPhase(self);

  // Clear weak refs
  clearWeakRefs(self);
  resetRefCounts(self);

  // Sweep phase
  profiler_begin(self->profiler, "sweep");
  for (int i = 0; i < heap->youngGeneration->sizeInCells; i++) {
    struct region_reference* currentObject = heap->youngGeneration->referenceLookup[i];
    struct object_info* currentObjectInfo = &heap->youngObjects[i];
      
    if (!currentObjectInfo->isValid)
      continue;

    // If the object exists in a region it 
    // must have valid entry
    assert(currentObjectInfo->isValid);
    assert(currentObjectInfo->type != OBJECT_TYPE_UNKNOWN);

    assert(currentObjectInfo->isMoved == false);
    
    if (atomic_exchange(&currentObjectInfo->isMarked, false) == false) {
      heap_sweep_an_object(heap, currentObjectInfo);
      continue;
    }
    
    struct region_reference* relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
    if (relocatedLocation)
      goto promotion_success;
    
    fixAddr(self, true);
    gc_trigger_old_collection(self, REPORT_PROMOTION_FAILURE);
    
    relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
    if (relocatedLocation)
      goto promotion_success;

    gc_trigger_full_collection(self, REPORT_PROMOTION_FAILURE, false);

    relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
    if (!relocatedLocation) {
      profiler_end(self->profiler);
      heap_report_printf(heap, "Old generation exhausted cannot promote");
      break;
    }

    promotion_success:
    doPromote(self, currentObject, currentObjectInfo, relocatedLocation);
  }
  profiler_end(self->profiler);

  profiler_begin(self->profiler, "fix-refs"); 
  fixAddr(self, false);
  profiler_end(self->profiler); 
  
  profiler_begin(self->profiler, "reset-relocation-info"); 
  gc_clear_old_to_young_card_table(self);
  
  for (int i = 0; i < self->heap->youngGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &self->heap->youngObjects[i];
    if (currentObjectInfo->isMoved) {
      struct object_info* relocated = currentObjectInfo->moveData.newLocationInfo;
      relocated->justMoved = false;
      relocated->moveData.oldLocation = NULL;
      relocated->moveData.oldLocationInfo = NULL;
      relocated->moveData.newLocation = NULL;
      relocated->moveData.newLocationInfo = NULL;
    }
    
    heap_reset_object_info(heap, currentObjectInfo);
    currentObjectInfo->isMoved = false;
  }
  profiler_end(self->profiler); 

  profiler_begin(self->profiler, "clearing-young-gen"); 
  region_wipe(self->heap->youngGeneration);
  region_move_bump_pointer_to_last(self->heap->youngGeneration);
  profiler_end(self->profiler); 
}

void gc_young_post_collect(struct gc_state* self) {
}

