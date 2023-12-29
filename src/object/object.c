#include <stdatomic.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <threads.h>

#include "macros.h"
#include "concurrency/mutex.h"
#include "concurrency/rwlock.h"
#include "managed_heap.h"
#include "memory/heap.h"
#include "memory/soc.h"
#include "object.h"
#include "context.h"
#include "panic.h"
#include "util/list_head.h"
#include "util/util.h"
#include "descriptor.h"
#include "bug.h"

SOC_DEFINE(syncStructureCache, SOC_DEFAULT_CHUNK_SIZE, struct object_sync_structure);

static _Atomic(struct object*)* getAtomicPtrToReference(struct object* self, size_t offset) {
  return (_Atomic(struct object*)*) ((char*) object_get_ptr(self) + offset);
}

struct object* object_resolve_forwarding(struct object* self) {
  while (self->forwardingPointer)
    self = self->forwardingPointer;
  return self;
}

static struct object* readPointerAt(struct object* self, size_t offset) {
  object_ptr_use_start(self);
  struct object* obj = atomic_load(getAtomicPtrToReference(self, offset));
  if (!obj)
    return NULL;
  
  struct object* forwarded = object_resolve_forwarding(obj);
  
  // Replace the pointer field into forwarded pointer
  if (obj != forwarded)
    atomic_compare_exchange_strong(getAtomicPtrToReference(self, offset), &obj, forwarded);
  obj = forwarded;
  object_ptr_use_end(self);
  return obj;
}

struct root_ref* object_read_reference(struct object* self, size_t offset) {
  struct root_ref* res = NULL;
  struct object* obj = readPointerAt(self, offset);
  if (!obj)
    return NULL;
  
  res = context_add_root_object(obj);
  return res;
}

static bool isEligibleForRememberedSet(struct object* parent, struct object* child) {
  // Add parent into child's generation's remembered set
  return child &&  parent->movePreserve.generationID != child->movePreserve.generationID && !list_is_valid(&parent->rememberedSetNode[child->movePreserve.generationID]);
}

void object_write_reference(struct object* self, size_t offset, struct object* child) {
  if (!descriptor_is_assignable_to(self, offset, child->movePreserve.descriptor))
    panic("Incompatible assignment");
  
  object_ptr_use_start(self);
  atomic_store(getAtomicPtrToReference(self, offset), child);
  object_ptr_use_end(self);
   
  // Add current object to child's generation remembered set
  if (isEligibleForRememberedSet(self, child)) {
    int childGenerationID = child->movePreserve.generationID;
    struct generation* childGeneration = &managed_heap_current->generations[childGenerationID];
    mutex_lock(&childGeneration->rememberedSetLock);
    list_add(&self->rememberedSetNode[childGenerationID], &childGeneration->rememberedSet);
    mutex_unlock(&childGeneration->rememberedSetLock);
  }
}

int object_init_synchronization_structs(struct object* self) {
  struct soc_chunk* chunk = NULL;
  struct object_sync_structure* data = soc_alloc_explicit(syncStructureCache, &chunk);
  if (!data)
    return -ENOMEM;
  data->sourceChunk = chunk;
  
  int res = 0;
  if ((res = mutex_init(&data->lock)) < 0)
    goto failure;
  if ((res = condition_init(&data->cond)) < 0) 
    goto failure;
  
  self->movePreserve.syncStructure = data;
  
failure:
  if (res < 0) {
    mutex_cleanup(&data->lock);
    condition_cleanup(&data->cond);
    soc_dealloc_explicit(syncStructureCache, chunk, data);
  }
  return res;
}

void object_cleanup(struct object* self, bool canRunFinalizer) {
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    if (list_is_valid(&self->rememberedSetNode[i]))
      list_del(&self->rememberedSetNode[i]);
  
  if (canRunFinalizer && self->movePreserve.descriptor->type == OBJECT_NORMAL)
    descriptor_run_finalizer_on(self->movePreserve.descriptor, self);
  
  if (self->movePreserve.syncStructure) {
    mutex_cleanup(&self->movePreserve.syncStructure->lock);
    condition_cleanup(&self->movePreserve.syncStructure->cond);
    soc_dealloc_explicit(syncStructureCache, self->movePreserve.syncStructure->sourceChunk, self->movePreserve.syncStructure);
  }
}

static void commonInit(struct object* self, struct descriptor* desc) {
  *self = (struct object) {
    .objectSize = descriptor_get_object_size(desc),
  };
  
  atomic_init(&self->isMarked, false);
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    list_init_as_invalid(&self->rememberedSetNode[i]);
}

void object_init(struct object* self, struct descriptor* desc) {
  commonInit(self, desc);
  self->movePreserve.descriptor = desc;
  self->movePreserve.foreverUniqueID = managed_heap_generate_object_id();
  self->movePreserve.overridePtr = NULL;
  self->movePreserve.syncStructure = NULL;
  descriptor_init_object(desc, self);
}

void object_fix_pointers(struct object* self) {
  BUG_ON(self->movePreserve.generationID < 0);
  
  object_for_each_field(self, ^int (struct object*, size_t offset) {
    readPointerAt(self, offset);
    return 0;
  });
}

struct object* object_move(struct object* self, struct heap* dest) {
  struct heap_block* newBlock = heap_alloc(dest, self->objectSize);
  if (!newBlock)
    return NULL;
  
  struct object* newBlockObj = &newBlock->objMetadata;
  self->forwardingPointer = newBlockObj;
  commonInit(newBlockObj, self->movePreserve.descriptor);
  heap_move(object_get_heap_block(self), newBlock);

  newBlockObj->movePreserve = self->movePreserve; 
  return newBlockObj;
}

int object_for_each_field(struct object* self, int (^iterator)(struct object* obj, size_t offset)) {
  return descriptor_for_each_offset(self, ^int (size_t offset) {
    return iterator(atomic_load(getAtomicPtrToReference(self, offset)), offset);
  });
}

const char* object_get_unique_name(struct object* self) {
  static thread_local char buffer[32 * 1024];
  snprintf(buffer, sizeof(buffer), "%s#%" PRIu64, descriptor_get_name(self->movePreserve.descriptor), self->movePreserve.foreverUniqueID);
  return buffer;
}

const char* object_ref_strength_tostring(enum reference_strength strength) {
  switch (strength) {
    case REF_STRONG:
      return "REF_STRONG";
    case REF_SOFT:
      return "REF_SOFT";
    case REF_WEAK:
      return "REF_WEAK";
    case REF_PHANTOM:
      return "REF_PHANTOM";
  }
  return NULL;
}

struct heap_block* object_get_heap_block(struct object* self) {
  return container_of(self, struct heap_block, objMetadata);
}

void* object_get_backing_ptr(struct object* self) {
  return heap_block_get_ptr(object_get_heap_block(self));
}

void* object_get_ptr(struct object* self) {
  if (self->movePreserve.overridePtr)
    return self->movePreserve.overridePtr->overridePtr;
  return object_get_backing_ptr(self);
}

// TODO: Convert this to lazy rwlock which init
// only if something want to block pointer access
// of current object (its just unnecessary that
// CopyingDMA blocks every pointer get on any heaps)
static struct rwlock ptrAccessLock = RWLOCK_INITIALIZER_PREFER_WRITER;

static thread_local int blockCount = 0;
void object_block_ptr(struct object* self) {
  UNUSED(self);
  blockCount++;
  if (blockCount == 1)
    rwlock_wrlock(&ptrAccessLock);
}

void object_unblock_ptr(struct object* self) {
  UNUSED(self);
  blockCount--;
  // Unbalanced block/unblock
  BUG_ON(blockCount < 0);
  if (blockCount == 0)
    rwlock_unlock(&ptrAccessLock);
}

void object_set_override_ptr(struct object* self, struct object_override_info* newPtr) {
  self->movePreserve.overridePtr = newPtr;
}

struct object_override_info* object_get_override_ptr(struct object* self) {
  return self->movePreserve.overridePtr;
}

static thread_local int useCount = 0;
void object_ptr_use_start(struct object* self) {
  UNUSED(self);
  useCount++;
  if (useCount == 1)
    rwlock_rdlock(&ptrAccessLock);
}

void object_ptr_use_end(struct object* self) {
  UNUSED(self);
  useCount--;
  // Unbalanced start/end
  BUG_ON(useCount < 0);
  if (useCount == 0)
    rwlock_unlock(&ptrAccessLock);
}






