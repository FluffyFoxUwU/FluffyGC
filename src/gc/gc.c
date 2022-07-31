#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/prctl.h>

#include "gc.h"
#include "gc_enums.h"
#include "young_collector.h"
#include "old_collector.h"
#include "full_collector.h"
#include "../heap.h"
#include "../root.h"
#include "../region.h"
#include "../root_iterator.h"
#include "../descriptor.h"
#include "../profiler.h"

static void gcTriggerYoungCollection(struct gc_state* self) {
  profiler_begin(self->profiler, "young-gc");
  
  profiler_begin(self->profiler, "pre-collect"); 
  bool res = gc_young_pre_collect(self);
  profiler_end(self->profiler); 

  if (!res) {
    heap_report_printf(self->heap, "Failure to run pre-collect task for old collection");
    goto failure;
  }

  profiler_begin(self->profiler, "collect"); 
  gc_young_collect(self);
  profiler_end(self->profiler); 
  
  profiler_begin(self->profiler, "post-collect"); 
  gc_young_post_collect(self);
  profiler_end(self->profiler);
  
  failure:
  profiler_end(self->profiler);
}

static void gcTriggerOldCollection(struct gc_state* self) {
  profiler_begin(self->profiler, "old-gc");
  
  profiler_begin(self->profiler, "pre-collect"); 
  bool res = gc_old_pre_collect(self);
  profiler_end(self->profiler); 

  if (!res) {
    heap_report_printf(self->heap, "Failure to run pre-collect task for old collection");
    goto failure;
  }

  profiler_begin(self->profiler, "collect"); 
  gc_old_collect(self);
  profiler_end(self->profiler); 
  
  profiler_begin(self->profiler, "post-collect"); 
  gc_old_post_collect(self);
  profiler_end(self->profiler);
  
  failure:
  profiler_end(self->profiler);
}

static void gcTriggerFullCollection(struct gc_state* self, bool isExplicit) {
  profiler_begin(self->profiler, "full-gc");
  
  profiler_begin(self->profiler, "pre-collect"); 
  bool res = gc_full_pre_collect(self, isExplicit);
  profiler_end(self->profiler); 

  if (!res) {
    heap_report_printf(self->heap, "Failure to run pre-collect task for old collection");
    goto failure;
  }

  profiler_begin(self->profiler, "collect"); 
  gc_full_collect(self, isExplicit);
  profiler_end(self->profiler); 
  
  profiler_begin(self->profiler, "post-collect"); 
  gc_full_post_collect(self, isExplicit);
  profiler_end(self->profiler);
  
  failure:
  profiler_end(self->profiler);
}

void gc_trigger_old_collection(struct gc_state* self, enum report_type reason) {
  heap_report_gc_cause(self->heap, reason);
  gcTriggerOldCollection(self);
}

void gc_trigger_young_collection(struct gc_state* self, enum report_type reason) {
  heap_report_gc_cause(self->heap, reason);
  gcTriggerYoungCollection(self);
}

void gc_trigger_full_collection(struct gc_state* self, enum report_type reason, bool isExplicit) {
  heap_report_gc_cause(self->heap, reason);
  gcTriggerFullCollection(self, isExplicit);
}

static void serve(struct gc_state* self, enum gc_request_type requestType, bool* shuttingDown) {
  struct heap* heap = self->heap;
  profiler_reset(self->profiler);
  profiler_start(self->profiler);
   
  bool canSignalCompletion = true;

  switch (requestType) {
    case GC_REQUEST_SHUTDOWN:
      *shuttingDown = true;
      break;
    case GC_REQUEST_COLLECT_YOUNG:
      gcTriggerYoungCollection(self);
      break;
    case GC_REQUEST_COLLECT_OLD:
      gcTriggerOldCollection(self);
      break;
    case GC_REQUEST_COLLECT_FULL:
      gcTriggerFullCollection(self, true);
      break;
    case GC_REQUEST_START_CONCURRENT:
      break;
    case GC_REQUEST_UNKNOWN:
      abort();
  }
  
  profiler_stop(self->profiler);
 
  if (canSignalCompletion) {
    profiler_dump(self->profiler, stderr);
    
    pthread_mutex_lock(&heap->gcCompletedLock);
    heap->gcCompleted = true;
    pthread_mutex_unlock(&heap->gcCompletedLock);
    pthread_cond_broadcast(&heap->gcCompletedCond);
  }
}

static void* mainThread(void* _self) {
  prctl(PR_SET_NAME, "GC-Thread");
  
  struct gc_state* self = _self;
  struct heap* heap = self->heap;
  bool shuttingDown = false;

  pthread_mutex_lock(&self->isGCReadyLock);
  self->isGCReady = true;
  pthread_mutex_unlock(&self->isGCReadyLock);

  pthread_mutex_lock(&heap->gcMayRunLock);
  pthread_cond_broadcast(&self->isGCReadyCond);

  while (!shuttingDown) {
    enum gc_request_type requestType;
    bool isRequested;
    
    heap->gcRequested = false;
    while (!heap->gcRequested)
      pthread_cond_wait(&heap->gcMayRunCond, &heap->gcMayRunLock); 

    isRequested = heap->gcRequested;
    requestType = heap->gcRequestedType;
     
    pthread_rwlock_wrlock(&heap->gcUnsafeRwlock); 
    size_t prevYoungSize = atomic_load(&heap->youngGeneration->usage);
    size_t prevOldSize = atomic_load(&heap->oldGeneration->usage);
    
    // Process the request
    serve(self, requestType, &shuttingDown);
    
    size_t youngSize = atomic_load(&heap->youngGeneration->usage);
    size_t oldSize = atomic_load(&heap->oldGeneration->usage);
    printf("[GC] Young usage: %.2f -> %.2f MiB\n", (double) prevYoungSize / 1024 / 1024,
                                                   (double) youngSize / 1024/ 1024);
    printf("[GC] Old   usage: %.2f -> %.2f MiB\n", (double) prevOldSize / 1024 / 1024,
                                                 (double) oldSize / 1024/ 1024);
    pthread_rwlock_unlock(&heap->gcUnsafeRwlock);
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
  self->isGCReady = false;
  self->fullGC.oldLookup = NULL;
  self->profiler = NULL;

  pthread_mutex_init(&self->isGCReadyLock, NULL);
  pthread_cond_init(&self->isGCReadyCond, NULL);

  self->profiler = profiler_new();
  if (!self->profiler)
    goto failure;

  self->fullGC.oldLookup = calloc(heap->oldGeneration->sizeInCells, sizeof(struct region_reference*));
  if (!self->fullGC.oldLookup)
    goto failure;

  if (pthread_create(&self->gcThread, NULL, mainThread, self) != 0)
    goto failure;
  self->isGCThreadRunning = true; 
  
  pthread_mutex_lock(&self->isGCReadyLock);
  if (!self->isGCReady)
    pthread_cond_wait(&self->isGCReadyCond, &self->isGCReadyLock);
  pthread_mutex_unlock(&self->isGCReadyLock);

  return self;

  failure:
  gc_cleanup(self);
  return NULL;
}

void gc_cleanup(struct gc_state* self) {
  if (!self)
    return;
  
  heap_call_gc_blocking(self->heap, GC_REQUEST_SHUTDOWN);
  
  if (self->isGCThreadRunning)
    pthread_join(self->gcThread, NULL);
  if (self->profiler)
    profiler_free(self->profiler);
  
  pthread_mutex_destroy(&self->isGCReadyLock);
  pthread_cond_destroy(&self->isGCReadyCond);

  free(self->fullGC.oldLookup);
  free(self);
}

void gc_fix_root(struct gc_state* self) {
  root_iterator_run(root_iterator_builder()
      ->heap(self->heap)
      ->consumer(^void (struct root_reference* ref, struct object_info* info) {
        if (info->justSweeped) {
          assert(ref->isWeak == true);
          ref->data = NULL;
          return;
        }
        if (!info->isMoved)
          return;
        
        assert(ref->data == info->moveData.oldLocation);
        assert(info->moveData.newLocation != info->moveData.oldLocation);
        ref->data = info->moveData.newLocation;
      })
      ->build()
  );
}

void gc_clear_young_to_old_card_table(struct gc_state* self) {
  for (int i = 0; i < self->heap->youngToOldCardTableSize; i++)
    atomic_store(&self->heap->youngToOldCardTable[i], false);
}

void gc_clear_old_to_young_card_table(struct gc_state* self) {
  for (int i = 0; i < self->heap->oldToYoungCardTableSize; i++)
    atomic_store(&self->heap->oldToYoungCardTable[i], false);
}

struct fixer_context {
  struct gc_state* gcState;
  gc_fixer_callback fixer;
};

// Default fixer
static void* defaultFixer(struct fixer_context* self, void* ptr) {
  // No need to update as its already NULL
  if (!ptr)
    return NULL; 

  struct region_reference* prevObject = heap_get_region_ref(self->gcState->heap, ptr);
  if (!prevObject)
    return ptr;

  struct object_info* prevObjectInfo = heap_get_object_info(self->gcState->heap, prevObject);
  struct region_reference* relocatedObject = prevObjectInfo->moveData.newLocation;
  
  // Object sweeped (usually from soft 
  // and weak refs)
  if (prevObjectInfo->justSweeped)
    return NULL;

  // The object not moved so no need
  // to fix it
  if (!prevObjectInfo->isMoved)
    return ptr;

  // Consistency check
  assert(prevObjectInfo->moveData.oldLocation == prevObject);
  
  return relocatedObject->data;
}

static void fixObjectRefsArray(struct fixer_context* self, struct object_info* objectInfo) {
  void** array = objectInfo->regionRef->data;

  for (int i = 0; i < objectInfo->typeSpecific.array.size; i++)
    array[i] = self->fixer(array[i]);
}

static void fixObjectRefsNormal(struct fixer_context* self, struct object_info* objectInfo) {
  struct descriptor* desc = objectInfo->typeSpecific.normal.desc;
  struct region_reference* ref = objectInfo->regionRef;
  
  // Opaque object
  if (!desc)
    return;

  for (int i = 0; i < desc->numFields; i++) {
    size_t offset = desc->fields[i].offset;
    void* ptr;
    region_read(ref->owner, ref, offset, &ptr, sizeof(void*));
 
    void* tmp = self->fixer(ptr);
    region_write(ref->owner, ref, offset, &tmp, sizeof(void*));
  }
}

static void fixObjectRefs(struct fixer_context* self, struct object_info* objectInfo) {
  assert(objectInfo->isValid);
  switch (objectInfo->type) {
    case OBJECT_TYPE_NORMAL:
      fixObjectRefsNormal(self, objectInfo);
      return;
    case OBJECT_TYPE_ARRAY:
      fixObjectRefsArray(self, objectInfo);
      return;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }

  abort();
}

void gc_fix_object_refs(struct gc_state* self, struct object_info* ref) {
  __block struct fixer_context ctx = {
    .gcState = self,
    .fixer = NULL
  };
  ctx.fixer =  ^void* (void* ptr) {
    return defaultFixer(&ctx, ptr);
  };

  fixObjectRefs(&ctx, ref);
}

void gc_fix_object_refs_custom(struct gc_state* self, struct object_info* ref, gc_fixer_callback call) {
  struct fixer_context ctx = {
    .gcState = self,
    .fixer = call
  };
  fixObjectRefs(&ctx, ref);
}

