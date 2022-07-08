#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "gc.h"
#include "young_collector.h"
#include "old_collector.h"
#include "full_collector.h"
#include "../heap.h"
#include "../root.h"
#include "../region.h"
#include "../root_iterator.h"
#include "../descriptor.h"

static void* mainThread(void* _self) {
  struct gc_state* self = _self;
  struct heap* heap = self->heap;
  bool shuttingDown = false;

  pthread_mutex_lock(&heap->gcMayRunLock);
  while (!shuttingDown) {
    enum gc_request_type requestType;
    bool isRequested;
    
    while (!heap->gcRequested || atomic_load(&heap->votedToBlockGC) > 0)
      pthread_cond_wait(&heap->gcMayRunCond, &heap->gcMayRunLock); 
    
    isRequested = heap->gcRequested;
    requestType = heap->gcRequestedType;

    size_t prevYoungSize = atomic_load(&heap->youngGeneration->usage);
    size_t prevOldSize = atomic_load(&heap->oldGeneration->usage);

    switch (requestType) {
      case GC_REQUEST_SHUTDOWN:
        shuttingDown = true;
        break;
      case GC_REQUEST_COLLECT_YOUNG:
        if (!gc_young_pre_collect(self)) {
          heap_report_printf(heap, "Failure to run pre-collect task for young collection");
          break;
        }
        gc_young_collect(self);
        gc_young_post_collect(self);
        break;
      case GC_REQUEST_COLLECT_OLD:
        if (!gc_old_pre_collect(self)) {
          heap_report_printf(heap, "Failure to run pre-collect task for old collection");
          break;
        }
        gc_old_collect(self);
        gc_old_post_collect(self);
        break;
      case GC_REQUEST_COLLECT_FULL:
        if (!gc_full_pre_collect(self)) {
          heap_report_printf(heap, "Failure to run pre-collect task for full collection");
          break;
        }
        gc_full_collect(self);
        gc_full_post_collect(self);
        break;
      case GC_REQUEST_START_CONCURRENT:
      case GC_REQUEST_CONTINUE:
        break;
      case GC_REQUEST_UNKNOWN:
        abort();
    }
    
    size_t youngSize = atomic_load(&heap->youngGeneration->usage);
    size_t oldSize = atomic_load(&heap->oldGeneration->usage);

    printf("[GC] Young usage: %.2f -> %.2f MiB\n", (double) prevYoungSize / 1024 / 1024,
                                                   (double) youngSize / 1024/ 1024);
    printf("[GC] Old   usage: %.2f -> %.2f MiB\n", (double) prevOldSize / 1024 / 1024,
                                                   (double) oldSize / 1024/ 1024);

    pthread_mutex_lock(&heap->gcCompletedLock);
    heap->gcCompleted = true;
    pthread_mutex_unlock(&heap->gcCompletedLock);
    pthread_cond_broadcast(&heap->gcCompletedCond);

    heap->gcRequested = false;
    heap->gcRequestedType = GC_REQUEST_UNKNOWN;  
  }
  pthread_mutex_unlock(&heap->gcMayRunLock);

  return NULL;
}

struct gc_state* gc_init(struct heap* heap) {
  struct gc_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
 
  self->heap = heap;
  self->statistics.youngGCCount = 0;
  self->statistics.youngGCTime = 0.0f;
  self->isGCThreadRunning = false;
  
  self->fullGC.oldLookup = calloc(heap->oldGeneration->sizeInCells, sizeof(struct region_reference*));
  if (!self->fullGC.oldLookup)
    goto failure;

  if (pthread_create(&self->gcThread, NULL, mainThread, self) != 0)
    goto failure;
  self->isGCThreadRunning = true; 

  return self;

  failure:
  gc_cleanup(self);
  return NULL;
}

void gc_cleanup(struct gc_state* self) {
  if (!self)
    return;

  heap_call_gc(self->heap, GC_REQUEST_SHUTDOWN);
  if (self->isGCThreadRunning)
    pthread_join(self->gcThread, NULL);
  
  free(self->fullGC.oldLookup);
  free(self);
}

void gc_fix_root(struct gc_state* self) {
  root_iterator_run(self->heap, NULL, ^bool (struct root_reference* ref, struct object_info* info) {
    if (info->isMoved) {
      assert(ref->data == info->moveData.oldLocation);
      assert(info->moveData.newLocation != info->moveData.oldLocation);
      ref->data = info->moveData.newLocation;
    }
    
    return true;
  });
}

void gc_clear_young_to_old_card_table(struct gc_state* self) {
  for (int i = 0; i < self->heap->youngToOldCardTableSize; i++)
    atomic_store(&self->heap->youngToOldCardTable[i], false);
}

void gc_clear_old_to_young_card_table(struct gc_state* self) {
  for (int i = 0; i < self->heap->oldToYoungCardTableSize; i++)
    atomic_store(&self->heap->oldToYoungCardTable[i], false);
}

void gc_trigger_old_collection(struct gc_state* self, enum report_type reason) {
  heap_report_gc_cause(self->heap, reason);
  
  if (!gc_old_pre_collect(self)) {
    heap_report_printf(self->heap, "Failure to run pre-collect task for old collection");
    return;
  }
  gc_old_collect(self);
  gc_old_post_collect(self);
}

void gc_trigger_full_collection(struct gc_state* self, enum report_type reason) {
  heap_report_gc_cause(self->heap, reason);
  
  if (!gc_full_pre_collect(self)) {
    heap_report_printf(self->heap, "Failure to run pre-collect task for full collection");
    return;
  }
  gc_full_collect(self);
  gc_full_post_collect(self);
}

struct fixer_context {
  struct gc_state* gcState;
  struct object_info* objects;
  struct object_info* sourceObjects;
  struct region* forGeneration;
  struct region* sourceGen;
  gc_fixer_callback fixer;
};

// Default fixer
static void* defaultFixer(struct fixer_context* self, void* ptr) {
  // No need to update as its already NULL
  if (!ptr)
    return NULL; 

  struct region_reference* prevObject = region_get_ref(self->sourceGen, ptr);
  if (!prevObject)
    return ptr;

  struct object_info* prevObjectInfo = &self->sourceObjects[prevObject->id];
  struct region_reference* relocatedObject = prevObjectInfo->moveData.newLocation;
  
  assert(prevObjectInfo->isMoved);
  assert(prevObjectInfo->moveData.oldLocation == prevObject);
  
  return relocatedObject->data;
}

static void fixObjectRefsArray(struct fixer_context* self, struct region_reference* ref) {
  assert(ref->owner == self->forGeneration);
 
  struct object_info* objectInfo = &self->objects[ref->id];
  void** array = objectInfo->regionRef->data;

  for (int i = 0; i < objectInfo->typeSpecific.pointerArray.size; i++)
    array[i] = self->fixer(array[i]);
}

static void fixObjectRefsNormal(struct fixer_context* self, struct region_reference* ref) {
  assert(ref->owner == self->forGeneration);
  struct object_info* objectInfo = &self->objects[ref->id];

  struct descriptor* desc = objectInfo->typeSpecific.normal.desc;
  for (int i = 0; i < desc->numFields; i++) {
    size_t offset = desc->fields[i].offset;
    void* ptr;
    region_read(ref->owner, ref, offset, &ptr, sizeof(void*));
 
    void* tmp = self->fixer(ptr);
    region_write(ref->owner, ref, offset, &tmp, sizeof(void*));
  }
}

static void fixObjectRefs(struct fixer_context* self, struct region_reference* ref) {
  assert(ref->owner == self->forGeneration);
  
  struct object_info* objectInfo = &self->objects[ref->id];
  assert(objectInfo->isValid);
  switch (objectInfo->type) {
    case OBJECT_TYPE_NORMAL:
      fixObjectRefsNormal(self, ref);
      return;
    case OBJECT_TYPE_ARRAY:
      fixObjectRefsArray(self, ref);
      return;
    case OBJECT_TYPE_OPAQUE:
      return;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }

  abort();
}

static struct fixer_context fixObjectCommon(struct gc_state* self, struct region* sourceGen, struct region* forGen, struct region_reference* ref) {
  struct object_info* objects;
  struct object_info* sourceObjects;
  if (forGen != NULL) {
    if (self->heap->oldGeneration == forGen)
      objects = self->heap->oldObjects;
    else if (self->heap->youngGeneration == forGen)
      objects = self->heap->youngObjects;
    else
      abort();
  } else {
    objects = NULL;  
  }

  if (sourceGen) {
    if (sourceGen == self->heap->youngGeneration)
      sourceObjects = self->heap->youngObjects;
    else if (sourceGen == self->heap->oldGeneration)
      sourceObjects = self->heap->oldObjects;
    else
      abort();
  } else {
    sourceObjects = NULL;  
  }
  
  struct fixer_context ctx = {
    .gcState = self,
    .objects = objects,
    .sourceObjects = sourceObjects,
    .sourceGen = sourceGen,
    .forGeneration = forGen,
    .fixer = NULL
  };
  return ctx;
}

void gc_fix_object_refs(struct gc_state* self, struct region* sourceGen, struct region* forGen, struct region_reference* ref) {
  assert(forGen);
  __block struct fixer_context ctx = fixObjectCommon(self, sourceGen, forGen, ref);

  ctx.fixer = ^void* (void* ptr) {
    return defaultFixer(&ctx, ptr);
  };
  fixObjectRefs(&ctx, ref);
}

void gc_fix_object_refs_custom(struct gc_state* self, struct region* sourceGen, struct region* forGen, struct region_reference* ref, gc_fixer_callback call) {
  assert(forGen);
  struct fixer_context ctx = fixObjectCommon(self, sourceGen, forGen, ref);
  ctx.fixer = call;
  fixObjectRefs(&ctx, ref);
}

