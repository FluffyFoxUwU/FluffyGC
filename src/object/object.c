#include <stdatomic.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "address_spaces.h"
#include "concurrency/mutex.h"
#include "config.h"
#include "gc/gc.h"
#include "managed_heap.h"
#include "memory/heap.h"
#include "memory/soc.h"
#include "object.h"
#include "context.h"
#include "panic.h"
#include "util/id_generator.h"
#include "util/list_head.h"
#include "util/util.h"
#include "descriptor.h"
#include "bug.h"

SOC_DEFINE(syncStructureCache, SOC_DEFAULT_CHUNK_SIZE, struct object_sync_structure);

static _Atomic(struct object*)* getAtomicPtrToReference(struct object* self, size_t offset) {
  return (_Atomic(struct object*)*) (self->dataPtr.ptr + offset);
}

struct object* object_resolve_forwarding(struct object* self) {
  while (self->forwardingPointer)
    self = self->forwardingPointer;
  return self;
}

static struct object* readPointerAt(struct object* self, size_t offset) {
  struct object* obj = atomic_load(getAtomicPtrToReference(self, offset));
  if (!obj)
    return NULL;
  
  struct object* forwarded = object_resolve_forwarding(obj);
  
  // Replace the pointer field into forwarded pointer
  if (obj != forwarded)
    atomic_compare_exchange_strong(getAtomicPtrToReference(self, offset), &obj, forwarded);
  obj = forwarded;
  return obj;
}

struct root_ref* object_read_reference(struct object* self, size_t offset) {
  context_block_gc();
  struct root_ref* res = NULL;
  struct object* obj = readPointerAt(self, offset);
  if (!obj)
    goto obj_is_null;
  
  res = context_add_root_object(obj);
  // Called in blocked GC state to ensure GC cycle not starting before
  // obj is checked for liveness
  gc_current->ops->postReadBarrier(obj);
obj_is_null:
  context_unblock_gc();
  return res;
}

static bool isEligibleForRememberedSet(struct object* parent, struct object* child) {
  return child &&  parent->movePreserve.generationID != child->movePreserve.generationID && !list_is_valid(&parent->rememberedSetNode[child->movePreserve.generationID]);
}

void object_write_reference(struct object* self, size_t offset, struct object* obj) {
  context_block_gc();
  if (!descriptor_is_assignable_to(self, offset, obj->movePreserve.descriptor))
    panic();
  
  struct object* old = atomic_exchange(getAtomicPtrToReference(self, offset), obj);
  
  // TODO: implement conditional write barriers
  gc_current->ops->postWriteBarrier(old);
  
  // Add current object to `obj`'s generation remembered set
  if (isEligibleForRememberedSet(self, obj)) {
    struct generation* target = &managed_heap_current->generations[obj->movePreserve.generationID];
    mutex_lock(&target->rememberedSetLock);
    list_add(&self->rememberedSetNode[obj->movePreserve.generationID], &target->rememberedSet);
    mutex_unlock(&target->rememberedSetLock);
  }
  context_unblock_gc();
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
  
  context_block_gc();
  self->syncStructure = data;
  context_unblock_gc();
  
failure:
  if (res < 0) {
    mutex_cleanup(&data->lock);
    condition_cleanup(&data->cond);
    soc_dealloc_explicit(syncStructureCache, chunk, data);
  }
  return res;
}

void object_cleanup(struct object* self, bool isDead) {
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    if (list_is_valid(&self->rememberedSetNode[i]))
      list_del(&self->rememberedSetNode[i]);
  
  // The object going to die anyway so give 
  // the direct pointer
  if (isDead && self->movePreserve.descriptor->type == OBJECT_NORMAL)
    descriptor_run_finalizer_on(self->movePreserve.descriptor, self);
    // self->movePreserve.descriptor->info.normal->api.param.finalizer((const void*) self->dataPtr.ptr);
  
  if (self->syncStructure) {
    mutex_cleanup(&self->syncStructure->lock);
    condition_cleanup(&self->syncStructure->cond);
    soc_dealloc_explicit(syncStructureCache, self->syncStructure->sourceChunk, self->syncStructure);
  }
}

struct userptr object_get_dma(struct root_ref* rootRef) {
  context_add_pinned_object(rootRef);
  return atomic_load(&rootRef->obj)->dataPtr;
}

int object_put_dma(struct root_ref* rootRef, struct userptr dma) {
  if (dma.ptr != atomic_load(&rootRef->obj)->dataPtr.ptr)
    return -EINVAL;
  context_remove_pinned_object(rootRef);
  return 0;
}

static void commonInit(struct object* self, struct descriptor* desc, void address_heap* data) {
  *self = (struct object) {
    .objectSize = descriptor_get_object_size(desc),
    .dataPtr = {data}
  };
  
  atomic_init(&self->isMarked, false);
  for (int i = 0; i < GC_MAX_GENERATIONS; i++)
    list_init_as_invalid(&self->rememberedSetNode[i]);
  descriptor_init_object(desc, self);
}

void object_init(struct object* self, struct descriptor* desc, void address_heap* data) {
  commonInit(self, desc, data);
  self->movePreserve.foreverUniqueID = id_generator_get();
}

void object_fix_pointers(struct object* self) {
  BUG_ON(self->movePreserve.generationID < 0);
  
  object_for_each_field(self, ^int (struct object*, size_t offset) {
    readPointerAt(self, offset);
    return 0;
  });
}

struct object* object_move(struct object* self, struct heap* dest) {
  struct heap_block* newBlock = heap_alloc(dest, descriptor_get_alignment(self->movePreserve.descriptor), self->objectSize);
  if (!newBlock)
    return NULL;
  
  struct object* newBlockObj = &newBlock->objMetadata;
  self->forwardingPointer = newBlockObj;
  commonInit(newBlockObj, self->movePreserve.descriptor, newBlock->dataPtr.ptr);
  memcpy(newBlockObj->dataPtr.ptr, self->dataPtr.ptr, self->objectSize);
  
  newBlockObj->movePreserve = self->movePreserve;
  
  // Poison self
  memset(&self->movePreserve, 0xF0, sizeof(self->movePreserve));
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    struct heap_block* block = container_of(self, struct heap_block, objMetadata);
    free(block->dataPtr.ptr);
    block->dataPtr.ptr = NULL;
  }
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
