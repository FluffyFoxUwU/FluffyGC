#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "descriptor.h"
#include "gc/gc.h"
#include "heap.h"
#include "config.h"
#include "reference.h"
#include "region.h"
#include "root.h"
#include "thread.h"

struct heap* heap_new(size_t youngSize, size_t oldSize, size_t metaspaceSize, int localFrameStackSize, float concurrentOldGCThreshold) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  self->metaspaceSize = metaspaceSize;
  self->metaspaceUsage = 0;
  self->gcCompleted = false;
  self->gcRequested = false;
  self->gcRequestedType = GC_REQUEST_UNKNOWN;
  self->votedToBlockGC = 0;
  self->threadsCount = 0;
  self->threadsListSize = 0;
  self->localFrameStackSize = localFrameStackSize;
  self->preTenurSize = youngSize;
  self->concurrentOldGCthreshold = concurrentOldGCThreshold;
  self->gcCompletedWaitingCount = 0;
  atomic_init(&self->votedToBlockGC, 0);

  pthread_mutex_init(&self->gcCompletedLock, NULL);
  pthread_mutex_init(&self->gcMayRunLock, NULL);
  
  pthread_cond_init(&self->gcCompletedCond, NULL);
  pthread_cond_init(&self->gcMayRunCond, NULL);
 
  pthread_rwlock_init(&self->lock, NULL);
  
  pthread_key_create(&self->currentThreadKey, NULL);
  
  self->metaspaceSize = metaspaceSize;
  self->youngGeneration = region_new(youngSize);
  if (!self->youngGeneration)
    goto failure;
  
  self->oldGeneration = region_new(oldSize);
  if (!self->oldGeneration)
    goto failure;
 
  size_t youngToOldCardTableSize = self->youngGeneration->sizeInCells / FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE;
  size_t oldToYoungCardTableSize = self->oldGeneration->sizeInCells / FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE;
  if (self->youngGeneration->sizeInCells % FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE > 0)
    youngToOldCardTableSize++;
  if (self->oldGeneration->sizeInCells % FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE > 0)
    oldToYoungCardTableSize++;

  self->oldToYoungCardTableSize = oldToYoungCardTableSize;
  self->youngToOldCardTableSize = youngToOldCardTableSize;

  self->oldToYoungCardTable = calloc(oldToYoungCardTableSize, sizeof(*self->oldToYoungCardTable));
  self->youngToOldCardTable = calloc(youngToOldCardTableSize, sizeof(*self->youngToOldCardTable));
  self->threads = calloc(0, sizeof(struct thread*));
  self->youngObjects = calloc(self->youngGeneration->sizeInCells, sizeof(struct object_info));
  self->oldObjects = calloc(self->oldGeneration->sizeInCells, sizeof(struct object_info));
  if (!self->threads ||
      !self->oldToYoungCardTable ||
      !self->youngToOldCardTable ||
      !self->youngObjects ||
      !self->oldObjects)
    goto failure;
  
  for (int i = 0; i < oldToYoungCardTableSize; i++)
    atomic_init(&self->oldToYoungCardTable[i], false);
  for (int i = 0; i < youngToOldCardTableSize; i++)
    atomic_init(&self->youngToOldCardTable[i], false);
  
  for (int i = 0; i < self->youngGeneration->sizeInCells; i++)
    atomic_init(&self->youngObjects[i].isMarked, false);
  
  for (int i = 0; i < self->oldGeneration->sizeInCells; i++)
    atomic_init(&self->oldObjects[i].isMarked, false);

  self->gcState = gc_init(self);
  if (!self->gcState)
    goto failure;

  return self;

  failure:
  heap_free(self);
  return NULL;
}

void heap_free(struct heap* self) {
  if (!self)
    return;

  if (self->threadsCount > 0)
    abort(); /* Please detach all threads */

  gc_cleanup(self->gcState);

  for (int i = 0; i < self->oldGeneration->sizeInCells; i++) {
    struct object_info* obj = &self->oldObjects[i];
    if (obj->isValid && obj)
      descriptor_release(obj->typeSpecific.normal.desc);
  }
  for (int i = 0; i < self->youngGeneration->sizeInCells; i++) {
    struct object_info* obj = &self->youngObjects[i];
    if (obj->isValid && obj)
      descriptor_release(obj->typeSpecific.normal.desc);
  }

  free(self->threads);
  free(self->oldToYoungCardTable);
  free(self->youngToOldCardTable);
  free(self->youngObjects);
  free(self->oldObjects);

  region_free(self->youngGeneration);
  region_free(self->oldGeneration);
  
  pthread_rwlock_destroy(&self->lock);
  
  pthread_mutex_destroy(&self->gcCompletedLock);
  pthread_mutex_destroy(&self->gcMayRunLock);
  
  pthread_cond_destroy(&self->gcCompletedCond);
  pthread_cond_destroy(&self->gcMayRunCond);
  
  pthread_key_delete(self->currentThreadKey);
  
  free(self);
}

struct descriptor* heap_descriptor_new(struct heap* self, struct descriptor_typeid id, size_t objectSize, int numFields, struct descriptor_field* fields) {
  pthread_rwlock_wrlock(&self->lock);
  size_t descriptorSize = sizeof(struct descriptor) + sizeof(struct descriptor_field) * numFields; 
  if (self->metaspaceUsage + descriptorSize > self->metaspaceSize)
    goto failure;

  self->metaspaceUsage += descriptorSize;
  struct descriptor* desc = descriptor_new(id, objectSize, numFields, fields);
  if (!desc) {
    self->metaspaceUsage -= descriptorSize;
    goto failure;
  }

  pthread_rwlock_unlock(&self->lock);
  return desc;

  failure:
  pthread_rwlock_unlock(&self->lock);
  return NULL;
}

void heap_descriptor_release(struct heap* self, struct descriptor* desc) {
  if (!desc)
    return;

  size_t descriptorSize = sizeof(struct descriptor) + sizeof(struct descriptor_field) * desc->numFields; 
  if (descriptor_release(desc)) {
    pthread_rwlock_wrlock(&self->lock);
    self->metaspaceUsage -= descriptorSize;
    pthread_rwlock_unlock(&self->lock);
  }
}

struct region* heap_get_region(struct heap* self, void* data) {
  assert(data);
  if (region_get_ref(self->youngGeneration, data))
    return self->youngGeneration;
  else if (region_get_ref(self->oldGeneration, data))
    return self->oldGeneration;
  return NULL;
}

struct region* heap_get_region2(struct heap* self, struct root_reference* data) {
  if (data->data->owner == self->youngGeneration)
    return self->youngGeneration;
  else if (data->data->owner == self->oldGeneration)
    return self->oldGeneration;
  return NULL;
}

struct object_info* heap_get_object_info(struct heap* self, struct region_reference* ref) {
  struct object_info* objects;
  struct region* region = ref->owner;
  if (region == self->youngGeneration) 
    objects = self->youngObjects;
  else if (region == self->oldGeneration)
    objects = self->oldObjects;
  else
    return NULL;

  return &objects[ref->id];
}

void heap_obj_write_ptr(struct heap* self, struct root_reference* object, size_t offset, struct root_reference* child) {
  // The object is from old generation
  // update card table
  struct region_reference* objectAsRegionRef = object->data;
  struct region* objectRegion = objectAsRegionRef->owner;

  struct object_info* objectList;
  if (objectRegion == self->youngGeneration)
    objectList = self->youngObjects;
  else
    objectList = self->oldObjects;

  assert(objectList[objectAsRegionRef->id].isValid);

  heap_enter_unsafe_gc(self);

  struct region* childRegion = heap_get_region2(self, child);
  if (childRegion != objectRegion) {
    if (childRegion == self->youngGeneration)
      atomic_store(&self->oldToYoungCardTable[objectAsRegionRef->id / FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE], true); 
    else
      atomic_store(&self->youngToOldCardTable[objectAsRegionRef->id / FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE], true); 
  }
  
  region_write(objectRegion, objectAsRegionRef, offset, (void*) &child->data->data, sizeof(void*));  
  heap_exit_unsafe_gc(self);
}

void heap_obj_write_data(struct heap* self, struct root_reference* object, size_t offset, void* data, size_t size) {
  heap_enter_unsafe_gc(self);
  region_write(heap_get_region2(self, object), object->data, offset, data, size);
  heap_exit_unsafe_gc(self);
}

void heap_obj_read_data(struct heap* self, struct root_reference* object, size_t offset, void* data, size_t size) {
  heap_enter_unsafe_gc(self);
  region_read(heap_get_region2(self, object), object->data, offset, data, size);
  heap_exit_unsafe_gc(self);
}

struct region_reference* heap_get_region_ref(struct heap* self, void* data) {
  struct region* region = heap_get_region(self, data);
  if (!region)
    return NULL;
  return region_get_ref(region, data);
}

struct root_reference* heap_obj_read_ptr(struct heap* self, struct root_reference* object, size_t offset) {
  heap_enter_unsafe_gc(self);
  
  struct region_reference* objectAsRegionRef = object->data;
  struct region* region = objectAsRegionRef->owner;

  void* ptr;
  region_read(region, objectAsRegionRef, offset, &ptr, sizeof(void*));

  struct root_reference* rootPtr = NULL;
  if (ptr) {
    struct thread* thread = heap_get_thread(self);
    struct region_reference* regionRef = heap_get_region_ref(self, ptr);
    assert(regionRef);
    rootPtr = thread_local_add(thread, regionRef);
  }

  heap_exit_unsafe_gc(self);
  return rootPtr;
}

void heap_wait_gc(struct heap* self) {
  pthread_mutex_lock(&self->gcCompletedLock);
  self->gcCompletedWaitingCount++;
  
  // Wait for GC
  while (!self->gcCompleted)
    pthread_cond_wait(&self->gcCompletedCond, &self->gcCompletedLock);

  self->gcCompletedWaitingCount--;
  if (self->gcCompletedWaitingCount == 0)
    self->gcCompleted = false;
  pthread_mutex_unlock(&self->gcCompletedLock);
}

void heap_enter_unsafe_gc(struct heap* self) {
  atomic_fetch_add(&self->votedToBlockGC, 1); 
}

void heap_exit_unsafe_gc(struct heap* self) {
  int prev = atomic_fetch_sub(&self->votedToBlockGC, 1);
  if (prev <= 0) {
    abort(); /* Exiting more than entered */
  } else if (prev == 1) {
    // Wake GC up and process the maybe 
    // pending request
    pthread_cond_broadcast(&self->gcMayRunCond);
  }
}

bool heap_is_attached(struct heap* self) {
  return pthread_getspecific(self->currentThreadKey) != NULL;
}

bool heap_attach_thread(struct heap* self) {
  // Find free entry
  // TODO: Make this not O(n) at worse
  //       (maybe some free space bitmap)
  pthread_rwlock_wrlock(&self->lock);
  int freePos = 0;
  for (; freePos < self->threadsListSize; freePos++)
    if (self->threads[freePos] == NULL)
      break;
  
  // Cant find free space
  if (freePos == self->threadsListSize)
    if (!heap_resize_threads_list_no_lock(self, self->threadsListSize + FLUFFYGC_HEAP_THREAD_LIST_STEP_SIZE))
      goto failure;

  struct thread* thread = thread_new(self, freePos, self->localFrameStackSize);
  if (!thread)
    goto failure;
  self->threads[freePos] = thread;

  self->threadsCount++;
  pthread_setspecific(self->currentThreadKey, thread);
  pthread_rwlock_unlock(&self->lock);
  return true;

  failure:
  pthread_rwlock_unlock(&self->lock);
  return false;
}

void heap_detach_thread(struct heap* self) {
  struct thread* thread = pthread_getspecific(self->currentThreadKey);
  
  pthread_rwlock_wrlock(&self->lock);
  self->threads[thread->id] = NULL;
  pthread_rwlock_unlock(&self->lock);

  thread_free(thread);
  
  self->threadsCount--;
  pthread_setspecific(self->currentThreadKey, NULL);
}

struct thread* heap_get_thread(struct heap* self) {
  return pthread_getspecific(self->currentThreadKey);
}

bool heap_resize_threads_list(struct heap* self, int newSize) {
  pthread_rwlock_wrlock(&self->lock);
  bool res = heap_resize_threads_list_no_lock(self, newSize);
  pthread_rwlock_unlock(&self->lock);
  return res;
}

bool heap_resize_threads_list_no_lock(struct heap* self, int newSize) {
  struct thread** newThreads = realloc(self->threads, sizeof(struct thread*) * newSize);
  if (!newThreads)
    return false;

  for (int i = self->threadsListSize; i < newSize; i++)
    newThreads[i] = NULL;

  self->threads = newThreads;
  self->threadsListSize = newSize;
  return true;
}

void heap_call_gc(struct heap* self, enum gc_request_type requestType) {
  pthread_mutex_lock(&self->gcMayRunLock);

  self->gcRequested = true;
  self->gcRequestedType = requestType;

  pthread_mutex_unlock(&self->gcMayRunLock); 
  pthread_cond_broadcast(&self->gcMayRunCond); 
}

static struct region_reference* tryAllocateOld(struct heap* self, size_t size) {   
  bool emergencyFullCollection = false;
  struct region_reference* regionRef;
 
  retry:;
  int retryCount = 0;
  for (; retryCount < FLUFFYGC_HEAP_MAX_OLD_ALLOC_RETRIES; retryCount++) {
    regionRef = region_alloc_or_fit(self->oldGeneration, size);
    if (regionRef != NULL)
      break;

    heap_exit_unsafe_gc(self);
    heap_report_gc_cause(self, REPORT_OLD_ALLOCATION_FAILURE);
    if (emergencyFullCollection) 
      heap_call_gc(self, GC_REQUEST_COLLECT_FULL);
    else
      heap_call_gc(self, GC_REQUEST_COLLECT_OLD);
    heap_wait_gc(self);
    heap_enter_unsafe_gc(self);
  }
  
  if (retryCount >= FLUFFYGC_HEAP_MAX_OLD_ALLOC_RETRIES && regionRef == NULL) {
    // Old GC cant free any space
    // trigger full collection
    if (!emergencyFullCollection) {
      emergencyFullCollection = true;
      goto retry;
    }
  
    // Old and Full GC can't free any more space
    heap_report_gc_cause(self, REPORT_OUT_OF_MEMORY);
    return NULL;
  }

  // Trigger concurrent GC
  float occupancyPercent = (float) self->oldGeneration->sizeInCells / 
                           (float) self->oldGeneration->bumpPointer;
  if (occupancyPercent >= self->concurrentOldGCthreshold)
    heap_call_gc(self, GC_REQUEST_START_CONCURRENT);

  return regionRef; 
}

static struct region_reference* tryAllocateYoung(struct heap* self, size_t size) {   
  int retryCount = 0;
  struct region_reference* regionRef;
  for (; retryCount < FLUFFYGC_HEAP_MAX_YOUNG_ALLOC_RETRIES; retryCount++) {
    regionRef = region_alloc(self->youngGeneration, size);
    if (regionRef != NULL)
      break;

    heap_report_gc_cause(self, REPORT_YOUNG_ALLOCATION_FAILURE);
    
    heap_exit_unsafe_gc(self);
    heap_call_gc(self, GC_REQUEST_COLLECT_YOUNG);
    heap_wait_gc(self);
    heap_enter_unsafe_gc(self);
  }
  
  // Young GC can't free any more space
  if (retryCount >= FLUFFYGC_HEAP_MAX_YOUNG_ALLOC_RETRIES && regionRef == NULL) {
    heap_report_gc_cause(self, REPORT_OUT_OF_MEMORY);
    return NULL;
  }

  return regionRef; 
}

void heap_reset_object_info(struct heap* self, struct object_info* object) {
  assert(self == object->owner || object->owner == NULL);
  assert(object->isValid);
  
  object->owner = self;

  object->typeSpecific.normal.desc = NULL;
  object->typeSpecific.pointerArray.size = 0;

  object->type = OBJECT_TYPE_UNKNOWN;
  object->finalizer = NULL;
  object->regionRef = NULL;
  object->isMoved = false;
  object->justMoved = false;
  object->moveData.oldLocation = NULL;
  object->moveData.newLocation = NULL;
  object->moveData.newLocationInfo = NULL;
  atomic_store(&object->isMarked, false);
}

static struct region_reference* tryAllocate(struct heap* self, size_t size, struct region** allocRegion) {
  struct region_reference* ref;
  if (region_get_actual_allocated_size(size) < self->preTenurSize) {
    ref = tryAllocateYoung(self, size);
    *allocRegion = self->youngGeneration;
  } else {
    ref = tryAllocateOld(self, size);
    *allocRegion = self->oldGeneration;
  }
  
  if (ref) {
    struct object_info* object = heap_get_object_info(self, ref);
    object->isValid = true;
    heap_reset_object_info(self, object);
    object->regionRef = ref;
  }
 
  assert(ref); 
  return ref;
}

static struct root_reference* commonNew(struct heap* self, size_t size) {
  struct region* allocRegion = NULL;
  struct region_reference* regionRef = tryAllocate(self, size, &allocRegion);
  if (!regionRef)
    return NULL;

  struct thread* thread = heap_get_thread(self);
  struct root_reference* ref = thread_local_add(thread, regionRef);
  if (!ref)
    goto failure_alloc_ref;

  return ref;

  failure_alloc_ref:
  region_dealloc(self->youngGeneration, regionRef);
  return NULL;
}

struct root_reference* heap_obj_new(struct heap* self, struct descriptor* desc) {
  heap_enter_unsafe_gc(self);
  struct root_reference* rootRef = commonNew(self, desc->objectSize);
  struct region_reference* regionRef = rootRef->data;
  struct region* allocRegion = heap_get_region2(self, rootRef);

  descriptor_init(desc, allocRegion, regionRef);
  
  struct object_info* objInfo = heap_get_object_info(self, regionRef); 
  objInfo->typeSpecific.normal.desc = desc;
  objInfo->type = OBJECT_TYPE_NORMAL;
  descriptor_acquire(desc);
  
  heap_exit_unsafe_gc(self);
  return rootRef;
}

struct root_reference* heap_obj_array(struct heap* self, int size) {
  heap_enter_unsafe_gc(self);
  struct root_reference* rootRef = commonNew(self, sizeof(void*) * size);
  struct region_reference* regionRef = rootRef->data;

  struct object_info* objInfo = heap_get_object_info(self, regionRef); 
  objInfo->type = OBJECT_TYPE_ARRAY;
  objInfo->typeSpecific.pointerArray.size = size;

  heap_exit_unsafe_gc(self);
  return rootRef;
}

void heap_report_printf(struct heap* self, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  heap_report_vprintf(self, fmt, args);
  va_end(args);
}

void heap_report_vprintf(struct heap* self, const char* fmt, va_list list) {
  char* buff = NULL;
  size_t buffSize = vsnprintf(NULL, 0, fmt, list);
  buff = malloc(buffSize);
  if (!buff) {
    fprintf(stderr, "[Heap %p] Error allocating buffer for printing error\n", self);
    return;
  }
  
  vsnprintf(buff, buffSize, fmt, list);
  fprintf(stderr, "[Heap %p] %s\n", self, buff);
  free(buff);
}

void heap_report_gc_cause(struct heap* self, enum report_type type) {
  switch (type) {
#   define X(name, str, ...) \
    case name: \
      heap_report_printf(self, "GC Trigger cause: %s\n", str); \
      break;
    REPORT_TYPES
#   undef X
  }
}



