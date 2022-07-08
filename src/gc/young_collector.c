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
#include "full_collector.h"
#include "gc.h"
#include "marker.h"

bool gc_young_pre_collect(struct gc_state* self) {
  return true;
}

typedef bool (^cardtable_iterator)(struct object_info* info, int index);
static bool iterateCardTable(struct gc_state* self, cardtable_iterator iterator) {
  for (int i = 0; i < self->heap->oldToYoungCardTableSize; i++) { 
    if (atomic_load(&self->heap->oldToYoungCardTable[i]) != true)
      continue;
    
    cellid_t rangeStart = i * FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE;
    
    for (int j = 0; j < FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE; j++) {
      struct object_info* info = &self->heap->oldObjects[rangeStart + j];
       
      if (!info->regionRef || info->isValid == false)
        continue;

      iterator(info, i);
    }
  }
  return true;
}

void gc_young_collect(struct gc_state* self) {
  bool promotionFailure = false;
  struct heap* heap = self->heap;  

  // Marking phase
  root_iterator_run(heap, heap->youngGeneration, ^bool (struct root_reference* ref, struct object_info* info) {
    gc_marker_mark(heap->youngGeneration, info);
    return true;
  });

  iterateCardTable(self, ^bool(struct object_info* info, int cardTableIndex) {
    gc_marker_mark(heap->youngGeneration, info);
    return true;
  });

  // Sweep phase
  for (int i = 0; i < heap->youngGeneration->sizeInCells; i++) {
    struct region_reference* currentObject = heap->youngGeneration->referenceLookup[i];
    struct object_info* currentObjectInfo = &heap->youngObjects[i];
      
    if (!currentObjectInfo->isValid)
      continue;

    // If the object exists in a region it 
    // must have valid entry
    assert(currentObjectInfo->isValid);
    assert(currentObjectInfo->type != OBJECT_TYPE_UNKNOWN);

    if (atomic_exchange(&currentObjectInfo->isMarked, false) == true) {
      if (promotionFailure)
        continue;

      //printf("Promoting %p\n", currentObject);
      struct region_reference* relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
      if (relocatedLocation == NULL) {
        //gc_trigger_old_collection(self, REPORT_PROMOTION_FAILURE);
        
        relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
        if (!relocatedLocation) {
          gc_trigger_full_collection(self, REPORT_PROMOTION_FAILURE);

          relocatedLocation = region_alloc_or_fit(self->heap->oldGeneration, currentObject->dataSize);
          if (!relocatedLocation) {
            promotionFailure = true;
            continue;
          }
        }
      }

      struct object_info* relocatedInfo = &self->heap->oldObjects[relocatedLocation->id];
      assert(relocatedInfo->isValid == false);
      
      *relocatedInfo = *currentObjectInfo;
      relocatedInfo->regionRef = relocatedLocation;
      relocatedInfo->justMoved = true;
      relocatedInfo->moveData.oldLocation = currentObject;

      heap_reset_object_info(self->heap, currentObjectInfo);
      currentObjectInfo->isValid = false;
      currentObjectInfo->isMoved = true;
      currentObjectInfo->moveData.oldLocation = currentObject;
      currentObjectInfo->moveData.newLocation = relocatedLocation;
      currentObjectInfo->moveData.newLocationInfo = relocatedInfo;

      // Copy over the data
      region_write(relocatedLocation->owner, relocatedLocation, 0, currentObject->data, currentObject->dataSize);
    } else {
      region_dealloc(currentObject->owner, currentObject);
      if (currentObjectInfo->type == OBJECT_TYPE_NORMAL)
        descriptor_release(currentObjectInfo->typeSpecific.normal.desc);
      heap_reset_object_info(self->heap, currentObjectInfo);
      currentObjectInfo->isValid = false;
    }
  }

  // Fix up addresses in promoted objects
  // No need to clear the object_info as
  // it will be cleared whenever new object
  // created at that location
  for (int i = 0; i < heap->youngGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->youngObjects[i];
    if (currentObjectInfo->isMoved)
      gc_fix_object_refs(self, heap->youngGeneration, heap->oldGeneration, currentObjectInfo->moveData.newLocation);
  }
   
  // Fix references in GC roots to
  // update the to young refs to
  // become to old refs (as youngs
  // is promoted)
  gc_fix_root(self);

  // Can be safely wiped
  if (!promotionFailure)
    region_wipe(self->heap->youngGeneration);
  else
    region_move_bump_pointer_to_last(self->heap->youngGeneration);
  
  // Reset `justMoved` flag
  for (int i = 0; i < heap->youngGeneration->sizeInCells; i++) {
    struct object_info* currentObjectInfo = &heap->youngObjects[i];
    if (currentObjectInfo->isMoved)
      currentObjectInfo->moveData.newLocationInfo->justMoved = false;
  }
  
  gc_clear_old_to_young_card_table(self);
}

void gc_young_post_collect(struct gc_state* self) {
}

