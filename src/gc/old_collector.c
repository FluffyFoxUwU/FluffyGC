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

#include "parallel_marker.h"
#include "parallel_heap_iterator.h"
#include "young_collector.h"
#include "gc.h"
#include "common.h"
#include "marker.h"
#include "cardtable_iterator.h"

bool gc_old_pre_collect(struct gc_state* self) {
  return true;
}

void gc_old_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  

  // Marking phase
  profiler_begin(self->profiler, "mark");
  gc_parallel_marker(self, false);
  profiler_end(self->profiler);

  // Clear non strong refs
  profiler_begin(self->profiler, "clear-weak-refs");
  gc_clear_non_strong_refs(self, false);
  profiler_end(self->profiler);

  // Sweep phase
  profiler_begin(self->profiler, "sweep");
  gc_parallel_heap_iterator_do(self, false, ^bool (struct object_info* currentObjectInfo, int idx) {
    atomic_store(&currentObjectInfo->strongRefCount, 0);      
    
    if (!currentObjectInfo->isValid ||
        atomic_exchange(&currentObjectInfo->isMarked, false) == true)
      return true;
    
    if (currentObjectInfo->justMoved) {
      currentObjectInfo->moveData.oldLocationInfo->isValid = false;
      heap_reset_object_info(heap, currentObjectInfo->moveData.oldLocationInfo);
    }

    heap_sweep_an_object(heap, currentObjectInfo);
    return true;
  }, NULL);
  profiler_end(self->profiler);
  
  profiler_begin(self->profiler, "post-sweep");
  region_move_bump_pointer_to_last(self->heap->oldGeneration);
  profiler_end(self->profiler);
}

void gc_old_post_collect(struct gc_state* self) {
}

