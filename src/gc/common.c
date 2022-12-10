#include <stdatomic.h>
#include <stdbool.h>

#include "common.h"
#include "gc.h"
#include "gc/parallel_heap_iterator.h"
#include "profiler.h"
#include "root.h"
#include "heap.h"
#include "region.h"
#include "root_iterator.h"
#include "cardtable_iterator.h"

void gc_clear_non_strong_refs(struct gc_state* gcState, bool isYoung) { 
  struct region* processRegion = gcState->heap->oldGeneration;
  struct object_info* cardTableObjects = gcState->heap->youngObjects;
  atomic_bool* cardTable = gcState->heap->youngToOldCardTable;
  size_t cardTableSize = gcState->heap->youngToOldCardTableSize;
  
  if (isYoung) {
    processRegion = gcState->heap->youngGeneration;
    cardTable = gcState->heap->oldToYoungCardTable;
    cardTableObjects = gcState->heap->oldObjects;
    cardTableSize = gcState->heap->oldToYoungCardTableSize;
  }
 
  profiler_begin(gcState->profiler, "clear-weak-a");
  gc_parallel_heap_iterator_do(gcState, isYoung, ^bool (struct object_info *info, int idx) {  
    if (!info->isValid || 
        atomic_load(&info->isMarked) == false)
      return true;

    if (gcState->isExplicit)
      gc_clear_soft_refs(gcState, info);
    gc_clear_weak_refs(gcState, info);
    return true;
  }, NULL);
  profiler_end(gcState->profiler);
  
  profiler_begin(gcState->profiler, "clear-weak-b");
  root_iterator_run(root_iterator_builder()
      ->heap(gcState->heap)
      ->only_in(processRegion)
      ->consumer(^void (struct root_reference* ref, struct object_info* info) {
        if (info->strongRefCount == 0)
          ref->data = NULL;
      })
      ->build()
  ); 
  profiler_end(gcState->profiler);
  
  profiler_begin(gcState->profiler, "clear-weak-c");
  cardtable_iterator_do(gcState->heap, cardTableObjects, cardTable, cardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    if (!info->isValid)
      return;
    
    if (gcState->isExplicit)
      gc_clear_soft_refs(gcState, info);
    gc_clear_weak_refs(gcState, info);
  });
  profiler_end(gcState->profiler);
}

