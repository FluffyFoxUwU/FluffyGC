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

bool gc_old_pre_collect(struct gc_state* self) {
  return true;
}

typedef void (^cardtable_iterator)(struct object_info* info, int index);
static bool iterateCardTable(struct gc_state* self, cardtable_iterator iterator) {
  for (int i = 0; i < self->heap->youngToOldCardTableSize; i++) { 
    if (atomic_load(&self->heap->youngToOldCardTable[i]) != true)
      continue;
    
    cellid_t rangeStart = i * FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE;
    
    for (int j = 0; j < FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE; j++) {
      struct object_info* info = &self->heap->youngObjects[rangeStart + j];
       
      if (!info->regionRef || info->isValid == false)
        continue;

      iterator(info, i);
    }
  }
  return true;
}

static void markPhase(struct gc_state* self) {
  struct heap* heap = self->heap;  

  // Marking phase
  root_iterator_run(heap, heap->oldGeneration, ^void (struct object_info* info) {
    gc_marker_mark(self, heap->oldGeneration, info);
  });

  iterateCardTable(self, ^void (struct object_info* info, int cardTableIndex) {
    gc_marker_mark(self, heap->oldGeneration, info);
  });
}

void gc_old_collect(struct gc_state* self) {
  struct heap* heap = self->heap;  

  markPhase(self);

  // Sweep phase
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
    
    heap_on_object_sweep(heap, currentObjectInfo);
  }

  region_move_bump_pointer_to_last(self->heap->oldGeneration);
}

void gc_old_post_collect(struct gc_state* self) {
  gc_clear_young_to_old_card_table(self);
}

