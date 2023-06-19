#include <stdatomic.h>
#include <stdio.h>
#include <strings.h>
#include <threads.h>

#include "generic.h"
#include "context.h"
#include "gc/gc.h"
#include "managed_heap.h"
#include "object/object.h"
#include "panic.h"
#include "util/list_head.h"
#include "memory/heap.h"
#include "util/util.h"
#include "vec.h"

static void clearRememberedSetFor(struct object* obj) {
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    if (list_is_valid(&obj->rememberedSetNode[i]))
      list_del(&obj->rememberedSetNode[i]);
}

static void recomputeRememberedSet(struct object* self) {
  clearRememberedSetFor(self);
  object_for_each_field(self, ^(struct object* child, size_t) {
    if (!child)
      return;
    
    if (child && self->movePreserve.generationID != child->movePreserve.generationID && !list_is_valid(&self->rememberedSetNode[child->movePreserve.generationID])) {
      struct generation* target = &managed_heap_current->generations[child->movePreserve.generationID];
      list_add(&self->rememberedSetNode[child->movePreserve.generationID], &target->rememberedSet);
    }
  });
}

static thread_local struct list_head promotedList;
static void postCollect(struct generation* gen) {
  struct list_head* current;
  struct list_head* next;
  
# define doGenerationStart() \
  for (int i = 0; i < managed_heap_current->generationCount; i++) { \
    struct generation* innerGen = gen; \
    if (!gen) \
      innerGen = &managed_heap_current->generations[i];
# define doGenerationEnd() \
    if (gen) \
      break; \
  }
  
  doGenerationStart()
  // Update so that no forward pointers exists in fromHeap
  list_for_each(current, &innerGen->toHeap->allocatedBlocks) {
    struct heap_block* objBlock = list_entry(current, struct heap_block, node);
    object_fix_pointers(&objBlock->objMetadata);
  }
  doGenerationEnd()
  
  gc_for_each_root_entry(gc_current, ^(struct root_ref* ref) {
    struct object* new;
    struct object* old = atomic_load(&ref->obj);
    
    new = object_resolve_forwarding(old);
    object_fix_pointers(new);
    
    atomic_store(&ref->obj, new);
  });
  
  doGenerationStart()
  list_for_each(current, &innerGen->rememberedSet) {
    struct object* obj = list_entry(current, struct object, rememberedSetNode[innerGen->genID]);
    object_fix_pointers(obj);
  }
  doGenerationEnd()
  
  list_for_each_safe(current, next, &promotedList) {
    struct object* obj = list_entry(current, struct object, inPromotionList);
    object_fix_pointers(obj);
  }
  
  doGenerationStart()
  list_for_each(current, &innerGen->toHeap->allocatedBlocks) {
    struct heap_block* objBlock = list_entry(current, struct heap_block, node);
    recomputeRememberedSet(&objBlock->objMetadata);
  }
  doGenerationEnd()
  
  gc_for_each_root_entry(gc_current, ^(struct root_ref* ref) {
    recomputeRememberedSet(atomic_load(&ref->obj));
  });
  
  list_for_each_safe(current, next, &promotedList) {
    struct object* obj = list_entry(current, struct object, inPromotionList);
    recomputeRememberedSet(obj);
    list_del(current);
  }
  
  doGenerationStart()
  list_for_each_safe(current, next, &innerGen->rememberedSet) {
    struct object* obj = list_entry(current, struct object, rememberedSetNode[innerGen->genID]);
    recomputeRememberedSet(obj);
  }
  doGenerationEnd()
  
  doGenerationStart()
  heap_clear(innerGen->fromHeap);
  swap(innerGen->fromHeap, innerGen->toHeap);
  doGenerationEnd()
  
# undef doGenerationStart
# undef doGenerationEnd
}

static size_t collectGeneration(struct generation* gen, struct generation** promoteTargetFailurePtr, bool fullGC, size_t* thisGenerationReclaimedSizePtr) {
  // Net worth of reclaimed space (dead objects)
  size_t globallyReclaimedSize = 0;
  
  // Reclaimed space within this generatio (promoted objects + dead objects)
  size_t thisGenerationReclaimedSize = 0;
  
  struct list_head* current;
  struct managed_heap* managedHeap = managed_heap_current;
  struct generation* promoteTo = NULL;
  
  int nextGenID = gen->genID + 1;
  if (nextGenID < managedHeap->generationCount)
    promoteTo = gen + 1;
  
  // Where promotion has failed
  struct generation* promoteTargetFailure = NULL;
  
  list_for_each(current, &gen->fromHeap->allocatedBlocks) {
    struct heap_block* objBlock = list_entry(current, struct heap_block, node);
    struct object* obj = &objBlock->objMetadata;
    int ageDelta = 1;
    size_t objectSize = objBlock->blockSize;
    
    // Dead object!
    if (!obj->isMarked) {
      globallyReclaimedSize += objectSize;
      thisGenerationReclaimedSize += objectSize;
      clearRememberedSetFor(obj);
      object_cleanup(obj, true);
      continue;
    }
    
    // Promote if there next generation
    if (obj->age + 1 >= gen->param.promotionAge && promoteTargetFailure == NULL && promoteTo && !fullGC) {
      struct object* newLocation;
      
      if (!(newLocation = object_move(obj, promoteTo->fromHeap))) {
        promoteTargetFailure = promoteTo;
        ageDelta = 0;
        goto object_is_alive;
      }
      
      clearRememberedSetFor(obj);
      
      newLocation->movePreserve.generationID = nextGenID;
      newLocation->age = 0;
      
      thisGenerationReclaimedSize += objectSize;
      gc_current->stat.promotedObjects++;
      gc_current->stat.promotedBytes += objectSize;
      
      list_add(&newLocation->inPromotionList, &promotedList);
      continue;
    }
    
object_is_alive:
    // Alive object send to toHeap
    clearRememberedSetFor(obj);
    
    struct object* newLocation;
    if (!(newLocation = object_move(obj, gen->toHeap)))
      panic();
    newLocation->age = MIN(newLocation->age + ageDelta, gen->param.promotionAge);
  }
  
  if (promoteTargetFailurePtr)
    *promoteTargetFailurePtr = promoteTargetFailure;
  if (thisGenerationReclaimedSizePtr)
    *thisGenerationReclaimedSizePtr = thisGenerationReclaimedSize;
  return globallyReclaimedSize;
}

typedef vec_t(struct object*) mark_state_stack;

static bool isEligibleForScanning(struct object* obj, int targetGenID) {
  if (!obj)
    return false;
  if (targetGenID == -1)
    return true;
  return obj->movePreserve.generationID == targetGenID;
}

// Basicly iterative DFS search
static int doDFSMark(int targetGenID, mark_state_stack* stack) {
  int res = 0;
  while (stack->length > 0) {
    struct object* current = vec_pop(stack);
    if (current->isMarked)
      continue;
    
    current->isMarked = true;
    object_for_each_field(current, ^(struct object* obj, size_t) {
      if (!isEligibleForScanning(obj, targetGenID))
        return;
      vec_push(stack, obj);
    });
  }
  return res;
}

int gc_generic_mark(struct generation* gen) {
  __block mark_state_stack currentPath;
  int res = 0;
  vec_init(&currentPath);
  if (vec_reserve(&currentPath, 64) < 0) {
    res = -ENOMEM;
    goto mark_failure;
  }
  
  int targetGenID = gen ? gen->genID : -1;
  gc_for_each_root_entry(gc_current, ^(struct root_ref* ref) {
    struct object* obj = atomic_load(&ref->obj);
    if (!isEligibleForScanning(obj, targetGenID))
      return;
    
    vec_push(&currentPath, obj);
    doDFSMark(targetGenID, &currentPath);
  });
  
  // We don't need scan remembered set when doing FullGC
  if (targetGenID >= 0) {
    struct list_head* current;
    list_for_each(current, &gen->rememberedSet) {
      struct object* obj = list_entry(current, struct object, rememberedSetNode[targetGenID]);
      object_for_each_field(obj, ^(struct object* child, size_t) {
        if (child)
          vec_push(&currentPath, child);
      });
      doDFSMark(targetGenID, &currentPath);
    }
  }
mark_failure:
  vec_deinit(&currentPath);
  return res;
}

static size_t doCollectAndMark(struct generation* gen) {
  struct generation* promoteTargetFailure = NULL;
  size_t thisGenerationReclaimedSizeTotal = 0;
  size_t reclaimedSize = 0;
  
  for (int i = 0; i <= gen->param.promotionAge; i++) {
    size_t thisGenerationReclaimedSize = 0;
    if (gc_generic_mark(gen) < 0)
      goto mark_failure;
    
    reclaimedSize += collectGeneration(gen, &promoteTargetFailure, false, &thisGenerationReclaimedSizeTotal);
    thisGenerationReclaimedSizeTotal += thisGenerationReclaimedSize;
    postCollect(gen);
    
    if (promoteTargetFailure) {
      if (gc_generic_mark(promoteTargetFailure) < 0)
        goto mark_failure;
      reclaimedSize += doCollectAndMark(promoteTargetFailure);
      promoteTargetFailure = NULL;
    }
    
    // Bail out if there more than enough space
    if (thisGenerationReclaimedSizeTotal >= gen->param.earlyPromoteSize)
      break;
  }

mark_failure:
  return reclaimedSize;
}

size_t gc_generic_collect(struct generation* gen) {
  list_head_init(&promotedList);
  
  if (!gen) {
    size_t reclaimedSize = 0;
    for (int i = 0; i < managed_heap_current->generationCount; i++) {
      if (gc_generic_mark(&managed_heap_current->generations[i]) < 0)
        goto mark_failure;
      reclaimedSize += collectGeneration(&managed_heap_current->generations[i], NULL, true, NULL);
      postCollect(&managed_heap_current->generations[i]);
    }
    
mark_failure:
    return reclaimedSize;
  }
  
  return doCollectAndMark(gen);
}

void gc_generic_compact(struct generation* gen) {
}
