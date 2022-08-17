#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "descriptor.h"
#include "gc/gc.h"
#include "gc/gc_enums.h"
#include "heap.h"
#include "config.h"
#include "region.h"
#include "root.h"
#include "thread.h"
#include "util.h"

struct heap* heap_new(size_t youngSize, size_t oldSize, size_t metaspaceSize, int localFrameStackSize, float concurrentOldGCThreshold, int globalRootSize) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  self->metaspaceSize = metaspaceSize;
  self->metaspaceUsage = 0;
  self->gcCompleted = false;
  self->gcRequested = false;
  self->gcRequestedType = GC_REQUEST_UNKNOWN;
  self->threadsCount = 0;
  self->threadsListSize = 0;
  self->localFrameStackSize = localFrameStackSize;
  self->preTenurSize = 64 * 1024;
  self->concurrentOldGCthreshold = concurrentOldGCThreshold;
  self->youngGeneration = NULL;
  self->oldGeneration = NULL;
  self->threads = NULL;
  self->oldToYoungCardTable = NULL;
  self->youngToOldCardTable = NULL;
  self->youngObjects = NULL;
  self->oldObjects = NULL;
  self->globalRoot = NULL;
  self->metaspaceSize = metaspaceSize;
  self->gcCompletedLockInited = false;
  self->gcMayRunLockInited = false;
  self->memoryExhaustionLockInited = false;
  self->callGCLockInited = false;
  self->gcCompletedCondInited = false;
  self->gcMayRunCondInited = false;
  self->lockInited = false;
  self->gcUnsafeRwlockInited = false;
  self->globalRootRWLockInited = false;
  self->currentThreadKeyInited = false;

  if (pthread_mutex_init(&self->gcCompletedLock, NULL) != 0)
    goto failure;
  self->gcCompletedLockInited = true;

  if (pthread_mutex_init(&self->gcMayRunLock, NULL) != 0)
    goto failure;
  self->gcMayRunLockInited = true;
  
  if (pthread_mutex_init(&self->memoryExhaustionLock, NULL) != 0)
    goto failure;
  self->memoryExhaustionLockInited = true;
  
  if (pthread_mutex_init(&self->callGCLock, NULL) != 0)
    goto failure; 
  self->callGCLockInited = true;
   
  if (pthread_cond_init(&self->gcCompletedCond, NULL) != 0)
    goto failure;
  self->gcCompletedCondInited = true;
  
  if (pthread_cond_init(&self->gcMayRunCond, NULL) != 0)
    goto failure;
  self->gcMayRunLockInited = true;
 
  if (pthread_rwlock_init(&self->lock, NULL) != 0)
    goto failure;
  self->lockInited = true;
  
  if (pthread_rwlock_init(&self->gcUnsafeRwlock, NULL) != 0)
    goto failure;
  self->gcUnsafeRwlockInited = true;
  
  if (pthread_rwlock_init(&self->globalRootRWLock, NULL) != 0)
    goto failure;
  self->globalRootRWLockInited = true;
  
  if (pthread_key_create(&self->currentThreadKey, NULL) != 0)
    goto failure;
  self->currentThreadKeyInited = true;

  self->globalRoot = root_new(globalRootSize);
  if (!self->globalRoot)
    goto failure;

  self->youngGeneration = region_new(youngSize);
  if (!self->youngGeneration)
    goto failure;
  
  self->oldGeneration = region_new(oldSize);
  if (!self->oldGeneration)
    goto failure;
 
  size_t youngToOldCardTableSize = self->youngGeneration->sizeInCells / CONFIG_CARD_TABLE_PER_BUCKET_SIZE;
  size_t oldToYoungCardTableSize = self->oldGeneration->sizeInCells / CONFIG_CARD_TABLE_PER_BUCKET_SIZE;
  if (self->youngGeneration->sizeInCells % CONFIG_CARD_TABLE_PER_BUCKET_SIZE > 0)
    youngToOldCardTableSize++;
  if (self->oldGeneration->sizeInCells % CONFIG_CARD_TABLE_PER_BUCKET_SIZE> 0)
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

  for (int i = 0; i < self->youngGeneration->sizeInCells; i++) {
    atomic_init(&self->youngObjects[i].isMarked, false);
    atomic_init(&self->youngObjects[i].strongRefCount, 0);
    heap_reset_object_info(self, &self->youngObjects[i]);
  }
  
  for (int i = 0; i < self->oldGeneration->sizeInCells; i++) {
    atomic_init(&self->oldObjects[i].isMarked, false);
    atomic_init(&self->oldObjects[i].strongRefCount, 0);
    heap_reset_object_info(self, &self->oldObjects[i]);
  }

  self->gcState = gc_init(self, util_get_core_count());
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
    if (obj->isValid && obj->type == OBJECT_TYPE_NORMAL)
      descriptor_release(obj->typeSpecific.normal.desc);
  }

  for (int i = 0; i < self->youngGeneration->sizeInCells; i++) {
    struct object_info* obj = &self->youngObjects[i];
    if (obj->isValid && obj->type == OBJECT_TYPE_NORMAL)
      descriptor_release(obj->typeSpecific.normal.desc);
  }

  root_free(self->globalRoot);

  free(self->threads);
  free(self->oldToYoungCardTable);
  free(self->youngToOldCardTable);
  free(self->youngObjects);
  free(self->oldObjects);

  region_free(self->youngGeneration);
  region_free(self->oldGeneration);
  
  if ((self->lockInited && pthread_rwlock_destroy(&self->lock) != 0) ||
      (self->gcUnsafeRwlockInited && pthread_rwlock_destroy(&self->gcUnsafeRwlock) != 0) ||
      (self->globalRootRWLockInited && pthread_rwlock_destroy(&self->globalRootRWLock) != 0) ||
      
      (self->gcCompletedLockInited && pthread_mutex_destroy(&self->gcCompletedLock) != 0) ||
      (self->gcMayRunLockInited && pthread_mutex_destroy(&self->gcMayRunLock) != 0) ||
      (self->memoryExhaustionLockInited && pthread_mutex_destroy(&self->memoryExhaustionLock) != 0) ||

      (self->gcCompletedCondInited && pthread_cond_destroy(&self->gcCompletedCond) != 0) ||
      (self->gcMayRunCondInited && pthread_cond_destroy(&self->gcMayRunCond) != 0) ||
    
      (self->currentThreadKeyInited && pthread_key_delete(self->currentThreadKey) != 0)) {
    abort();
  }
  
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

  // Attempt to unregister twice
  if (atomic_exchange(&desc->alreadyUnregistered, true) == true)
    abort();

  size_t descriptorSize = sizeof(struct descriptor) + sizeof(struct descriptor_field) * desc->numFields; 
  descriptor_release(desc);

  pthread_rwlock_wrlock(&self->lock);
  self->metaspaceUsage -= descriptorSize;
  pthread_rwlock_unlock(&self->lock);
}

struct region* heap_get_region(struct heap* self, void* data) {
  assert(data);
  if (region_get_cellid(self->youngGeneration, data) != -1)
    return self->youngGeneration;
  else if (region_get_cellid(self->oldGeneration, data) != -1)
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

bool heap_can_record_in_cardtable(struct heap* self, struct object_info* obj, size_t offset, struct object_info* data) {
  return true;

  switch (obj->type) {
    case OBJECT_TYPE_ARRAY:
      return obj->typeSpecific.array.strength == REFERENCE_STRENGTH_STRONG;
    case OBJECT_TYPE_NORMAL: 
    {
      struct descriptor* desc = obj->typeSpecific.normal.desc;
      struct descriptor_field* field = &desc->fields[descriptor_get_index_from_offset(desc, offset)];
      return field->strength == REFERENCE_STRENGTH_STRONG;
    }
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }
  abort();
}

static void writePtrCommon(struct heap* self, struct root_reference* object, size_t offset, struct root_reference* toWrite) {
  struct region_reference* objectAsRegionRef = object->data;
  struct region* objectRegion = objectAsRegionRef->owner;
  struct object_info* info = heap_get_object_info(self, objectAsRegionRef);
  assert(info->isValid);

  if (heap_can_record_in_cardtable(self, info, offset, heap_get_object_info(self, toWrite->data))) {
    struct region* childRegion = heap_get_region2(self, toWrite);
    if (childRegion != objectRegion) {
      if (childRegion == self->youngGeneration)
        atomic_store(&self->oldToYoungCardTable[objectAsRegionRef->id / CONFIG_CARD_TABLE_PER_BUCKET_SIZE], true); 
      else
        atomic_store(&self->youngToOldCardTable[objectAsRegionRef->id / CONFIG_CARD_TABLE_PER_BUCKET_SIZE], true); 
    }  
  }

  region_write(objectRegion, objectAsRegionRef, offset, (void*) &toWrite->data->data, sizeof(void*));  
}

static struct root_reference* readPtrCommon(struct heap* self, struct root_reference* object, size_t offset) { 
  struct region_reference* objectAsRegionRef = object->data;
  struct region* region = objectAsRegionRef->owner;

  void* ptr;
  region_read(region, objectAsRegionRef, offset, &ptr, sizeof(void*));

  if (!ptr)
    return NULL;
  
  struct thread* thread = heap_get_thread_data(self)->thread;
  struct region_reference* regionRef = heap_get_region_ref(self, ptr);
  assert(regionRef);
  return thread_local_add(thread, regionRef);
}

void heap_obj_write_ptr(struct heap* self, struct root_reference* object, size_t offset, struct root_reference* child) {
  heap_enter_unsafe_gc(self);
  assert(object->isValid);

  struct object_info* objectInfo = heap_get_object_info(self, object->data);
  assert(objectInfo);
  if (objectInfo->type != OBJECT_TYPE_NORMAL)
    abort();

  writePtrCommon(self, object, offset, child);
  heap_exit_unsafe_gc(self);
}

struct root_reference* heap_obj_read_ptr(struct heap* self, struct root_reference* object, size_t offset) {
  heap_enter_unsafe_gc(self);
  assert(object->isValid);
  
  struct object_info* objectInfo = heap_get_object_info(self, object->data);
  assert(objectInfo);
  if (objectInfo->type != OBJECT_TYPE_NORMAL)
    abort();
  
  struct root_reference* ref = readPtrCommon(self, object, offset);
  heap_exit_unsafe_gc(self);
  return ref;
}

void heap_array_write(struct heap* self, struct root_reference* object, int index, struct root_reference* child) {
  heap_enter_unsafe_gc(self);
  assert(object->isValid);
  
  struct object_info* objectInfo = heap_get_object_info(self, object->data);
  assert(objectInfo);
  if (objectInfo->type != OBJECT_TYPE_ARRAY)
    abort();

  writePtrCommon(self, object, sizeof(void*) * index, child);
  heap_exit_unsafe_gc(self); 
}

struct root_reference* heap_array_read(struct heap* self, struct root_reference* object, int index) {
  heap_enter_unsafe_gc(self);
  assert(object->isValid);
  
  struct object_info* objectInfo = heap_get_object_info(self, object->data);
  assert(objectInfo);
  if (objectInfo->type != OBJECT_TYPE_ARRAY)
    abort();
  
  struct root_reference* ref = readPtrCommon(self, object, sizeof(void*) * index);
  heap_exit_unsafe_gc(self);
  return ref;
}

bool heap_is_attached(struct heap* self) {
  return pthread_getspecific(self->currentThreadKey) != NULL;
}

bool heap_attach_thread(struct heap* self) {
  // Find free entry
  // TODO: Make this not O(n) at worse
  //       (maybe some free space bitmap)
  pthread_rwlock_rdlock(&self->gcUnsafeRwlock);
  pthread_rwlock_wrlock(&self->lock);
  int freePos = 0;
  for (; freePos < self->threadsListSize; freePos++)
    if (self->threads[freePos] == NULL)
      break;
  
  struct thread* thread = NULL;
  struct thread_data* data = NULL;

  // Cant find free space
  if (freePos == self->threadsListSize)
    if (!heap_resize_threads_list_no_lock(self, self->threadsListSize + CONFIG_THREAD_LIST_STEP_SIZE))
      goto failure;

  thread = thread_new(self, freePos, self->localFrameStackSize);
  if (!thread)
    goto failure;
  self->threads[freePos] = thread;

  self->threadsCount++;

  data = malloc(sizeof(*data));
  if (!data)
    goto failure;

  data->thread = thread;
  data->numberOfTimeEnteredUnsafeGC = 0;

  pthread_setspecific(self->currentThreadKey, data);
  pthread_rwlock_unlock(&self->lock);
  pthread_rwlock_unlock(&self->gcUnsafeRwlock);
  return true;

  failure:
  thread_free(thread);
  free(data);
  pthread_rwlock_unlock(&self->lock);
  pthread_rwlock_unlock(&self->gcUnsafeRwlock);
  return false;
}

void heap_detach_thread(struct heap* self) {
  struct thread_data* data = heap_get_thread_data(self);
  
  pthread_rwlock_wrlock(&self->lock);
  self->threads[data->thread->id] = NULL;
  pthread_rwlock_unlock(&self->lock);

  thread_free(data->thread);
  free(data);

  self->threadsCount--;
  pthread_setspecific(self->currentThreadKey, NULL);
}

struct thread_data* heap_get_thread_data(struct heap* self) {
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

void heap_wait_gc_no_lock(struct heap* self) {
  while (!self->gcCompleted)
    pthread_cond_wait(&self->gcCompletedCond, &self->gcCompletedLock);
}

void heap_wait_gc(struct heap* self) {
  pthread_mutex_lock(&self->gcCompletedLock);
  heap_wait_gc_no_lock(self);
  pthread_mutex_unlock(&self->gcCompletedLock);
}

void heap_enter_unsafe_gc(struct heap* self) {
  struct thread_data* data = heap_get_thread_data(self);
  if (data->numberOfTimeEnteredUnsafeGC == 0)
    pthread_rwlock_rdlock(&self->gcUnsafeRwlock);
  data->numberOfTimeEnteredUnsafeGC++;
}

void heap_exit_unsafe_gc(struct heap* self) {
  struct thread_data* data = heap_get_thread_data(self);
  if (data->numberOfTimeEnteredUnsafeGC == 1)
    pthread_rwlock_unlock(&self->gcUnsafeRwlock);
  data->numberOfTimeEnteredUnsafeGC--;
}

void heap_call_gc(struct heap* self, enum gc_request_type requestType) {
  pthread_mutex_lock(&self->callGCLock);

  pthread_mutex_lock(&self->gcMayRunLock);
  if (self->gcRequested && self->gcRequestedType == requestType) {
    pthread_mutex_unlock(&self->gcMayRunLock); 
    goto request_already_sent;
  }
  
  self->gcRequested = true;
  self->gcRequestedType = requestType;

  pthread_mutex_unlock(&self->gcMayRunLock);  
  pthread_cond_broadcast(&self->gcMayRunCond); 

  request_already_sent:
  pthread_mutex_unlock(&self->callGCLock);
}

void heap_call_gc_blocking(struct heap* self, enum gc_request_type requestType) {
  pthread_mutex_lock(&self->callGCLock);

  if (self->gcRequested && self->gcRequestedType == requestType)
    goto request_already_sent;
  
  self->gcCompleted = false;

  pthread_mutex_lock(&self->gcMayRunLock);
  self->gcRequested = true;
  self->gcRequestedType = requestType;
  pthread_mutex_unlock(&self->gcMayRunLock); 
  pthread_cond_broadcast(&self->gcMayRunCond); 
  
  request_already_sent:
  heap_wait_gc(self);
  pthread_mutex_unlock(&self->callGCLock);
}

static void onHeapExhaustion(struct heap* self) {
  heap_report_printf(self, "Heap memory exhausted");
}

static void tryFixOOM(struct heap* self) {
  heap_report_gc_cause(self, REPORT_OUT_OF_MEMORY);
  heap_call_gc_blocking(self, GC_REQUEST_COLLECT_FULL);
}

static void tryFixYoungExhaustion(struct heap* self) {
  heap_report_gc_cause(self, REPORT_YOUNG_ALLOCATION_FAILURE);
  heap_call_gc_blocking(self, GC_REQUEST_COLLECT_YOUNG);
}

static void tryFixOldExhaustion(struct heap* self) {
  heap_report_gc_cause(self, REPORT_OLD_ALLOCATION_FAILURE);
  heap_call_gc_blocking(self, GC_REQUEST_COLLECT_OLD);
}

static struct region_reference* tryAllocateOld(struct heap* self, size_t size) {
  struct region_reference* regionRef = region_alloc_or_fit(self->oldGeneration, size);
  if (regionRef != NULL)
    goto allocation_success_first_try;
  
  heap_exit_unsafe_gc(self);
  pthread_mutex_lock(&self->memoryExhaustionLock);
  heap_enter_unsafe_gc(self);
  
  regionRef = region_alloc_or_fit(self->oldGeneration, size);
  if (regionRef != NULL)
    goto allocation_success;
  
  heap_exit_unsafe_gc(self);
  tryFixOldExhaustion(self);
  heap_enter_unsafe_gc(self);

  regionRef = region_alloc_or_fit(self->oldGeneration, size);
  if (regionRef != NULL)
    goto allocation_success;
  
  heap_exit_unsafe_gc(self);
  tryFixOOM(self);
  heap_enter_unsafe_gc(self);

  regionRef = region_alloc_or_fit(self->oldGeneration, size);
  if (!regionRef) {
    onHeapExhaustion(self);
  pthread_mutex_unlock(&self->memoryExhaustionLock);
    return NULL;
  }

  allocation_success:
  pthread_mutex_unlock(&self->memoryExhaustionLock);
  allocation_success_first_try:
  return regionRef; 
}

static struct region_reference* tryAllocateYoung(struct heap* self, size_t size) {   
  struct region_reference* regionRef = region_alloc(self->youngGeneration, size);
  if (regionRef != NULL)
    goto allocation_success_first_try;
  
  heap_exit_unsafe_gc(self);
  pthread_mutex_lock(&self->memoryExhaustionLock);
  heap_enter_unsafe_gc(self);
  
  regionRef = region_alloc(self->youngGeneration, size);
  if (regionRef != NULL)
    goto allocation_success;
  
  heap_exit_unsafe_gc(self);
  tryFixYoungExhaustion(self);
  heap_enter_unsafe_gc(self);
  
  regionRef = region_alloc(self->youngGeneration, size);  
  if (regionRef != NULL)
    goto allocation_success;

  heap_exit_unsafe_gc(self);
  tryFixOOM(self);
  heap_enter_unsafe_gc(self);

  regionRef = region_alloc(self->youngGeneration, size);  
  if (regionRef == NULL) {
    onHeapExhaustion(self);
    return NULL;
  }
  
  allocation_success:
  pthread_mutex_unlock(&self->memoryExhaustionLock);
  allocation_success_first_try:
  return regionRef; 
}

void heap_reset_object_info(struct heap* self, struct object_info* object) {
  object->owner = self;

  object->typeSpecific.normal.desc = NULL;
  object->typeSpecific.array.size = 0;
  object->typeSpecific.array.strength = REFERENCE_STRENGTH_STRONG;

  object->type = OBJECT_TYPE_UNKNOWN;
  object->finalizer = NULL;
  object->regionRef = NULL;
  object->isMoved = false;
  object->justMoved = false;
  object->moveData.oldLocation = NULL;
  object->moveData.oldLocationInfo = NULL;
  object->moveData.newLocation = NULL;
  object->moveData.newLocationInfo = NULL;
  atomic_store(&object->strongRefCount, 0);
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
  
  if (!ref)
    return NULL;
  
  struct object_info* object = heap_get_object_info(self, ref);
  object->isValid = true;
  heap_reset_object_info(self, object);
  object->regionRef = ref;
 
  /*
  // Trigger concurrent GC
  float occupancyPercent = (float) atomic_load(&self->oldGeneration->usage) /
                           (float) self->oldGeneration->sizeInCells;
  if (occupancyPercent >= self->concurrentOldGCthreshold)
    heap_call_gc(self, GC_REQUEST_START_CONCURRENT);
  */

  return ref;
}

static void commonAttrInit(struct heap* self, struct object_info* info) {
}

static struct root_reference* commonNew(struct heap* self, size_t size) {
  struct region* allocRegion = NULL;
  struct region_reference* regionRef = tryAllocate(self, size, &allocRegion);
  if (!regionRef)
    return NULL;

  struct thread* thread = heap_get_thread_data(self)->thread;
  struct root_reference* ref = thread_local_add(thread, regionRef);
  if (!ref)
    goto failure_alloc_ref;

  return ref;

  failure_alloc_ref:
  region_dealloc(self->youngGeneration, regionRef);
  return NULL;
}

struct root_reference* heap_obj_opaque_new(struct heap* self, size_t size) {
  heap_enter_unsafe_gc(self);
  struct root_reference* rootRef = commonNew(self, size);
  if (!rootRef)
    goto failure;

  struct region_reference* regionRef = rootRef->data;

  struct object_info* objInfo = heap_get_object_info(self, regionRef); 
  objInfo->type = OBJECT_TYPE_NORMAL;
  objInfo->typeSpecific.normal.desc = NULL;
  commonAttrInit(self, objInfo);

  heap_exit_unsafe_gc(self);
  return rootRef;
  
  failure:
  heap_exit_unsafe_gc(self);
  return NULL;
}

struct root_reference* heap_obj_new(struct heap* self, struct descriptor* desc) {
  if (atomic_load(&desc->alreadyUnregistered) == true)
    abort();
  
  heap_enter_unsafe_gc(self);
  descriptor_acquire(desc);
  struct root_reference* rootRef = commonNew(self, desc->objectSize);
  if (!rootRef)
    goto failure;

  struct region_reference* regionRef = rootRef->data;
  struct region* allocRegion = regionRef->owner;

  descriptor_init(desc, allocRegion, regionRef);
  
  struct object_info* objInfo = heap_get_object_info(self, regionRef); 
  objInfo->typeSpecific.normal.desc = desc;
  objInfo->type = OBJECT_TYPE_NORMAL;
  commonAttrInit(self, objInfo);
  
  heap_exit_unsafe_gc(self);
  return rootRef;
  
  failure:
  heap_exit_unsafe_gc(self);
  return NULL;
}

struct root_reference* heap_array_new(struct heap* self, int size) {
  heap_enter_unsafe_gc(self);

  struct root_reference* rootRef = commonNew(self, sizeof(void*) * size);
  if (!rootRef)
    goto failure;
  
  struct region_reference* regionRef = rootRef->data;

  struct object_info* objInfo = heap_get_object_info(self, regionRef); 
  objInfo->type = OBJECT_TYPE_ARRAY;
  objInfo->typeSpecific.array.size = size;
  objInfo->typeSpecific.array.strength = REFERENCE_STRENGTH_STRONG;
  commonAttrInit(self, objInfo);
  memset(regionRef->data, 0, regionRef->dataSize);

  heap_exit_unsafe_gc(self);
  return rootRef;
  
  failure:
  heap_exit_unsafe_gc(self);
  return NULL;
}

void heap_report_printf(struct heap* self, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  heap_report_vprintf(self, fmt, args);
  va_end(args);
}

void heap_report_vprintf(struct heap* self, const char* fmt, va_list list) {
  if (1)
    return;

  char* buff = NULL;
  size_t buffSize = vsnprintf(NULL, 0, fmt, list);
  buff = malloc(buffSize + 1);
  if (!buff) {
    fprintf(stderr, "[Heap %p] Error allocating buffer for printing error\n", self);
    return;
  }
  
  vsnprintf(buff, buffSize + 1, fmt, list);
  fprintf(stderr, "[Heap %p] %s\n", self, buff);
  free(buff);
}

void heap_report_gc_cause(struct heap* self, enum report_type type) {
  switch (type) {
#   define X(name, str, ...) \
    case name: \
      heap_report_printf(self, "GC Trigger cause: %s", str); \
      break;
    REPORT_TYPES
#   undef X
  }
}

void heap_sweep_an_object(struct heap* self, struct object_info* obj) {
  switch (obj->type) {
    case OBJECT_TYPE_NORMAL:
      descriptor_release(obj->typeSpecific.normal.desc);
      break;
    case OBJECT_TYPE_ARRAY:
      break;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }
 
  region_dealloc(obj->regionRef->owner, obj->regionRef);
  heap_reset_object_info(self, obj);
  obj->isValid = false;
}

bool heap_is_array(struct heap* self, struct root_reference* ref) {
  switch (heap_get_object_info(self, ref->data)->type) {
    case OBJECT_TYPE_ARRAY:
      return true;
    case OBJECT_TYPE_NORMAL:
      return false;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }
  abort();
}

const char* heap_tostring_object_type(struct heap* self, enum object_type type) {
  switch (type) {
#   define X(t, str, ...) case t: return str;
    OBJECT_TYPES
#   undef X
  }
  abort();
}



