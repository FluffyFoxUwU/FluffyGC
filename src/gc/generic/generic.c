#include <stdatomic.h>
#include <stdio.h>
#include <threads.h>

#include "generic.h"
#include "bug.h"
#include "context.h"
#include "gc/gc.h"
#include "managed_heap.h"
#include "object/object.h"
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
    
    if (child->generationID < 0)
      *(volatile int*) child->dataPtr.ptr;
    if (child && self->generationID != child->generationID && !list_is_valid(&self->rememberedSetNode[child->generationID])) {
      struct generation* target = &context_current->managedHeap->generations[child->generationID];
      list_add(&self->rememberedSetNode[child->generationID], &target->rememberedSet);
    }
  });
}

static void objectIsAlive(struct generation* gen, struct object* oldObj) {
  struct object* newLocation;
  clearRememberedSetFor(oldObj);
  
  if (!(newLocation = object_move(oldObj, gen->toHeap)))
    BUG();
  newLocation->age++;
}

static thread_local struct list_head promotedList;
static void postCollect(struct generation* gen) {
  struct managed_heap* managedHeap = context_current->managedHeap;
  int currentGenID = indexof(managedHeap->generations, gen);
  
  struct list_head* current;
  struct list_head* next;
  
  // Update so that no forward pointers exists in fromHeap
  list_for_each(current, &gen->toHeap->allocatedBlocks) {
    struct heap_block* objBlock = list_entry(current, struct heap_block, node);
    object_fix_pointers(&objBlock->objMetadata);
  }
  
  gc_for_each_root_entry(gc_current, ^(struct root_ref* ref) {
    struct object* new;
    struct object* old = atomic_load(&ref->obj);
    
    new = object_resolve_forwarding(old);
    object_fix_pointers(new);
    
    atomic_store(&ref->obj, new);
  });
  
  list_for_each_safe(current, next, &promotedList) {
    struct object* obj = list_entry(current, struct object, inPromotionList);
    object_fix_pointers(obj);
  }
  
  list_for_each(current, &gen->rememberedSet) {
    struct object* obj = list_entry(current, struct object, rememberedSetNode[currentGenID]);
    object_fix_pointers(obj);
  }
  
  list_for_each(current, &gen->toHeap->allocatedBlocks) {
    struct heap_block* objBlock = list_entry(current, struct heap_block, node);
    recomputeRememberedSet(&objBlock->objMetadata);
  }
  
  gc_for_each_root_entry(gc_current, ^(struct root_ref* ref) {
    recomputeRememberedSet(atomic_load(&ref->obj));
  });
  
  list_for_each_safe(current, next, &promotedList) {
    struct object* obj = list_entry(current, struct object, inPromotionList);
    recomputeRememberedSet(obj);
    list_del(current);
  }
  
  list_for_each_safe(current, next, &gen->rememberedSet) {
    struct object* obj = list_entry(current, struct object, rememberedSetNode[currentGenID]);
    *(volatile int*) obj->dataPtr.ptr;
    recomputeRememberedSet(obj);
  }
  
  heap_clear(gen->fromHeap);
  swap(gen->fromHeap, gen->toHeap);
  
  printf("[GC] Heap stat: ");
  for (int i = 0; i < managedHeap->generationCount; i++) {
    struct generation* gen = &managedHeap->generations[i];
    printf("Gen%d: %10zu bytes / %10zu bytes   ", i, gen->fromHeap->usage, gen->fromHeap->size);
  }
  puts("");
}

static size_t collectGeneration(struct generation* gen) {
  list_head_init(&promotedList);
  
  size_t reclaimedSize = 0;
  struct list_head* current;
  struct list_head* next;
  struct managed_heap* managedHeap = context_current->managedHeap;
  
  list_for_each_safe(current, next, &gen->fromHeap->allocatedBlocks) {
    struct heap_block* objBlock = list_entry(current, struct heap_block, node);
    struct object* obj = &objBlock->objMetadata;
    
    // Dead object!
    if (!obj->isMarked) {
      clearRememberedSetFor(obj);
      object_cleanup(obj);
      reclaimedSize += objBlock->blockSize;
      continue;
    }
    
    // Promote if there next generation
    if (obj->age + 1 >= gen->param.promotionAge && obj->generationID + 1 < managedHeap->generationCount) {
      struct object* newLocation;
      int nextGenID = obj->generationID + 1;
      struct generation* promoteTo = &managedHeap->generations[nextGenID];
      
      if (!(newLocation = object_move(obj, promoteTo->fromHeap))) {
        obj->age = 0;
        objectIsAlive(gen, obj);
        continue;
      }
      
      clearRememberedSetFor(obj);
      
      newLocation->generationID = nextGenID;
      newLocation->age = 0;
      
      list_add(&newLocation->inPromotionList, &promotedList);
      continue;
    }
    
object_is_alive:
    // Alive object send to toHeap
    objectIsAlive(gen, obj);
  }
  
  postCollect(gen);
  return reclaimedSize;
}

static size_t doCollectAndMark(struct generation* gen) {
  if (gc_generic_mark(gen) < 0)
    return 0;
  return gc_generic_collect(gen);
}

typedef vec_t(struct object*) mark_state_stack;

// Basicly iterative DFS search
static int doDFSMark(int targetGenID, mark_state_stack* stack) {
  int res = 0;
  while (stack->length > 0) {
    struct object* current = vec_pop(stack);
    if (current->isMarked)
      continue;
    
    current->isMarked = true;
    object_for_each_field(current, ^(struct object* obj, size_t) {
      if (!obj || obj->generationID != targetGenID)
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
  
  struct managed_heap* managedHeap = context_current->managedHeap;
  int currentGenID = indexof(managedHeap->generations, gen);
  
  gc_for_each_root_entry(gc_current, ^(struct root_ref* ref) {
    struct object* obj = atomic_load(&ref->obj);
    if (!obj || obj->generationID != currentGenID)
      return;
    
    vec_push(&currentPath, obj);
    doDFSMark(currentGenID, &currentPath);
  });
  
  struct list_head* current;
  list_for_each(current, &gen->rememberedSet) {
    struct object* obj = list_entry(current, struct object, rememberedSetNode[currentGenID]);
    object_for_each_field(obj, ^(struct object* child, size_t) {
      if (child)
        vec_push(&currentPath, child);
    });
    doDFSMark(currentGenID, &currentPath);
  }
mark_failure:
  vec_deinit(&currentPath);
  return res;
}

size_t gc_generic_collect_and_mark(struct generation* gen) {
  if (gen)
    return doCollectAndMark(gen);
  
  size_t reclaimedSize = 0;
  struct managed_heap* managedHeap = context_current->managedHeap;
  for (int i = 0; i < managedHeap->generationCount; i++)
    reclaimedSize += doCollectAndMark(&managedHeap->generations[i]);
  return reclaimedSize;
}

size_t gc_generic_collect(struct generation* gen) {
  return collectGeneration(gen);
}

void gc_generic_compact(struct generation* gen) {
}
