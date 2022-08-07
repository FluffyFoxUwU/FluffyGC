#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "gc.h"
#include "gc_enums.h"
#include "reference_iterator.h"
#include "thread_pool.h"
#include "util.h"
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
  bool prev = self->isExplicit;
  self->isExplicit = isExplicit;
  
  profiler_begin(self->profiler, "full-gc");
  
  profiler_begin(self->profiler, "pre-collect"); 
  bool res = gc_full_pre_collect(self);
  profiler_end(self->profiler); 

  if (!res) {
    heap_report_printf(self->heap, "Failure to run pre-collect task for old collection");
    goto failure;
  }

  profiler_begin(self->profiler, "collect"); 
  gc_full_collect(self);
  profiler_end(self->profiler); 
  
  profiler_begin(self->profiler, "post-collect"); 
  gc_full_post_collect(self);
  profiler_end(self->profiler);
  
  failure:
  profiler_end(self->profiler);
  self->isExplicit = prev;
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
  profiler_reset(self->profiler);
  profiler_start(self->profiler);
   
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

  self->isExplicit = false;
  profiler_stop(self->profiler);
  
  //profiler_dump(self->profiler, stderr);
}

static void* mainThread(void* _self) {
  util_set_thread_name("GC-Thread");
  
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
    //size_t prevYoungSize = atomic_load(&heap->youngGeneration->usage);
    //size_t prevOldSize = atomic_load(&heap->oldGeneration->usage);
    
    // Process the request
    serve(self, requestType, &shuttingDown);
    
    /*
    size_t youngSize = atomic_load(&heap->youngGeneration->usage);
    size_t oldSize = atomic_load(&heap->oldGeneration->usage);
    printf("[GC] Young usage: %.2f -> %.2f MiB\n", (double) prevYoungSize / 1024 / 1024,
                                                   (double) youngSize / 1024/ 1024);
    printf("[GC] Old   usage: %.2f -> %.2f MiB\n", (double) prevOldSize / 1024 / 1024,
                                                 (double) oldSize / 1024/ 1024);
    */
    pthread_rwlock_unlock(&heap->gcUnsafeRwlock);

    pthread_mutex_lock(&heap->gcCompletedLock);
    heap->gcCompleted = true;
    pthread_mutex_unlock(&heap->gcCompletedLock);
    pthread_cond_broadcast(&heap->gcCompletedCond);
  }
  pthread_mutex_unlock(&heap->gcMayRunLock);

  return NULL;
}

struct gc_state* gc_init(struct heap* heap, int workerCount) {
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
  self->isExplicit = false;
  self->workerPool= NULL; 

  pthread_mutex_init(&self->isGCReadyLock, NULL);
  pthread_cond_init(&self->isGCReadyCond, NULL);

  self->profiler = profiler_new();
  if (!self->profiler)
    goto failure;

  self->fullGC.oldLookup = calloc(heap->oldGeneration->sizeInCells, sizeof(struct region_reference*));
  if (!self->fullGC.oldLookup)
    goto failure;
  
  self->workerPool = thread_pool_new(workerCount, "GC-Worker-");
  if (!self->workerPool)
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
  
  thread_pool_free(self->workerPool);
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
  assert(prevObjectInfo);

  // The object not moved so no need
  // to fix it
  if (!prevObjectInfo->isMoved)
    return ptr;

  // Consistency check
  assert(prevObjectInfo->moveData.oldLocation == prevObject);
  
  return relocatedObject->data;
}

void gc_fix_object_refs(struct gc_state* self, struct object_info* ref) {
  __block struct fixer_context ctx = {
    .gcState = self,
    .fixer = NULL
  };
  ctx.fixer = ^void* (void* ptr) {
    return defaultFixer(&ctx, ptr);
  };
  
  reference_iterator_run(reference_iterator_builder()
      ->object(ref)
      ->heap(self->heap)
      ->consumer(^void (struct object_info* child, void* ptr, size_t offset) {
        void* tmp = ctx.fixer(ptr);
        region_write(ref->regionRef->owner, ref->regionRef, offset, &tmp, sizeof(void*));
      })
      ->build()
  );
}

void gc_fix_object_refs_custom(struct gc_state* self, struct object_info* ref, gc_fixer_callback call) {
  reference_iterator_run(reference_iterator_builder()
      ->object(ref)
      ->heap(self->heap)
      ->consumer(^void (struct object_info* child, void* ptr, size_t offset) {
        void* tmp = call(ptr);
        region_write(ref->regionRef->owner, ref->regionRef, offset, &tmp, sizeof(void*));
      })
      ->build()
  );
}

void gc_clear_weak_refs(struct gc_state* self, struct object_info* object) {
  reference_iterator_run(reference_iterator_builder()
      ->object(object)
      ->heap(self->heap)
      ->ignore_soft(true)
      ->ignore_strong(true)
      ->consumer(^void (struct object_info* child, void* ptr, size_t offset) {
        if (child->strongRefCount == 0) {
          void* tmp = NULL;
          region_write(object->regionRef->owner, object->regionRef, offset, &tmp, sizeof(void*));
        }
      })
      ->build()
  );
}

void gc_clear_soft_refs(struct gc_state* self, struct object_info* object) {
  reference_iterator_run(reference_iterator_builder()
      ->object(object)
      ->heap(self->heap)
      ->ignore_weak(true)
      ->ignore_strong(true)
      ->consumer(^void (struct object_info* child, void* ptr, size_t offset) {
        if (atomic_load(&child->strongRefCount) == 0) {
          void* tmp = NULL;
          region_write(object->regionRef->owner, object->regionRef, offset, &tmp, sizeof(void*));
        }
      })
      ->build()
  );
}

