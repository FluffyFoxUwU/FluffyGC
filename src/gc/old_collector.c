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

#include "young_collector.h"
#include "gc.h"
#include "marker.h"
#include "cardtable_iterator.h"

bool gc_old_pre_collect(struct gc_state* self) {
  return true;
}


static void markPhase(struct gc_state* self) {
  profiler_begin(self->profiler, "mark");
  
  struct heap* heap = self->heap;  

  // Marking phase
  root_iterator_run(heap, heap->oldGeneration, ^void (struct object_info* info) {
    gc_marker_mark(self, heap->oldGeneration, info);
  });

  cardtable_iterator_do(heap, heap->youngObjects, heap->youngToOldCardTable, heap->youngToOldCardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    gc_marker_mark(self, heap->oldGeneration, info);
  });
  
  profiler_end(self->profiler);
}

void gc_old_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  

  markPhase(self);

  // Sweep phase
  profiler_begin(self->profiler, "sweep");
  for (int i = 0; i < heap->oldGeneration->sizeInCells; i++) {
    struct region_reference* currentObject = heap->oldGeneration->referenceLookup[i];
    struct object_info* currentObjectInfo = &heap->oldObjects[i];
    
    // No need to do anything else :3
    if (!currentObject ||
        !currentObjectInfo->isValid ||
        atomic_exchange(&currentObjectInfo->isMarked, false) == true ||
        currentObjectInfo->justMoved) {
      continue;
    }
    
    if (currentObjectInfo->justMoved) {
      struct object_info* oldInfo = currentObjectInfo->moveData.oldLocationInfo;
      assert(oldInfo->isMoved);
      assert(oldInfo->moveData.newLocationInfo == currentObjectInfo);
      assert(oldInfo->moveData.oldLocationInfo == oldInfo);
      
      heap_reset_object_info(heap, oldInfo);
    }
    
    heap_sweep_an_object(heap, currentObjectInfo);
  }
  profiler_end(self->profiler);

  profiler_begin(self->profiler, "post-sweep");
  region_move_bump_pointer_to_last(self->heap->oldGeneration);
  profiler_end(self->profiler);
}

void gc_old_post_collect(struct gc_state* self) {
}

