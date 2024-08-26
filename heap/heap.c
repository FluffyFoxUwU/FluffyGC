#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <flup/bug.h>
#include <flup/core/panic.h>
#include <flup/thread/thread.h>
#include <flup/thread/thread_local.h>
#include <flup/concurrency/mutex.h>
#include <flup/container_of.h>
#include <flup/core/logger.h>
#include <flup/data_structs/list_head.h>

#include "heap.h"
#include "gc/driver.h"
#include "gc/gc.h"
#include "heap/generation.h"
#include "heap/thread.h"
#include "memory/alloc_context.h"
#include "memory/alloc_tracker.h"
#include "object/descriptor.h"

#define HEAP_ALLOC_RETRY_COUNT 5

struct heap* heap_new(size_t size) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct heap) {
    .threads = FLUP_LIST_HEAD_INIT(self->threads)
  };
  
  if (!(self->threadListLock = flup_mutex_new()))
    goto failure;
  
  if (!(self->gen = generation_new(size)))
    goto failure;
  self->gen->ownerHeap = self;
  if (!(self->currentThread = flup_thread_local_new(NULL)))
    goto failure;
  
  if (!heap_attach_thread(self))
    goto failure;
  
  // Heap is ready, unpause the GC driver
  gc_driver_unpause(self->gen->gcState->driver);
  return self;

failure:
  heap_free(self);
  return NULL;
}

struct thread* heap_get_current_thread(struct heap* self) {
  return (struct thread*) flup_thread_local_get(self->currentThread);
}

static void removeThread(struct heap* self, struct thread* thread) {
  flup_mutex_lock(self->threadListLock);
  flup_list_del(&thread->node);
  flup_mutex_unlock(self->threadListLock);
  
  thread_free(thread);
}

void heap_free(struct heap* self) {
  if (!self)
    return;
  
  gc_perform_shutdown(self->gen->gcState);
  flup_thread_local_free(self->currentThread);
  flup_list_head* current;
  flup_list_head *next;
  flup_list_for_each_safe(&self->threads, current, next)
    removeThread(self, flup_list_entry(current, struct thread, node));
  generation_free(self->gen);
  flup_mutex_free(self->threadListLock);
  free(self);
}

struct root_ref* heap_new_root_ref_unlocked(struct heap* self, struct alloc_unit* obj) {
  return thread_new_root_ref_no_gc_block(heap_get_current_thread(self), obj);
}

void heap_root_unref(struct heap* self, struct root_ref* ref) {
  heap_block_gc(self);
  thread_unref_root_no_gc_block(heap_get_current_thread(self), ref);
  heap_unblock_gc(self);
}

struct root_ref* heap_alloc_with_descriptor(struct heap* self, struct descriptor* desc, size_t extraSize) {
  struct root_ref* ref = heap_alloc(self, desc->objectSize + extraSize);
  if (!ref)
    return NULL;
  
  descriptor_init_object(desc, extraSize, ref->obj->data);
  atomic_store(&ref->obj->desc, desc);
  return ref;
}

struct root_ref* heap_alloc(struct heap* self, size_t size) {
  struct root_ref* ref = thread_prealloc_root_ref(heap_get_current_thread(self));
  if (!ref)
    return NULL;
  
  heap_block_gc(self);
  struct alloc_unit* newObj = generation_alloc(self->gen, size);
  for (int i = 0; i < HEAP_ALLOC_RETRY_COUNT && newObj == NULL; i++) {
    pr_info("Allocation failed trying calling GC #%d, GC was %srunning", i + 1, atomic_load(&self->gen->gcState->cycleInProgress) ? "" : "not ");
    heap_unblock_gc(self);
    gc_start_cycle(self->gen->gcState);
    heap_block_gc(self);
    newObj = generation_alloc(self->gen, size);
  }
  
  // Heap is actually OOM-ed
  if (!newObj) {
    free(ref);
    heap_unblock_gc(self);
    return NULL;
  }
  
  thread_new_root_ref_from_prealloc_no_gc_block(heap_get_current_thread(self), ref, newObj);
  gc_on_allocate(newObj, self->gen);
  heap_unblock_gc(self);
  return ref;
}

void heap_block_gc(struct heap* self) {
  gc_block(self->gen->gcState, heap_get_current_thread(self));
}

void heap_unblock_gc(struct heap* self) {
  gc_unblock(self->gen->gcState, heap_get_current_thread(self));
}

struct alloc_context* heap_get_alloc_context(struct heap* self) {
  return heap_get_current_thread(self)->allocContext;
}

void heap_iterate_threads(struct heap* self, void (^iterator)(struct thread* thrd)) {
  flup_mutex_lock(self->threadListLock);
  flup_list_head* current;
  flup_list_for_each(&self->threads, current)
    iterator(flup_list_entry(current, struct thread, node));
  flup_mutex_unlock(self->threadListLock);
}

struct thread* heap_attach_thread(struct heap* self) {
  struct thread* thrd = thread_new(self);
  if (!thrd)
    return NULL;
  
  flup_mutex_lock(self->threadListLock);
  flup_list_add_head(&self->threads, &thrd->node);
  flup_mutex_unlock(self->threadListLock);
  
  flup_thread_local_set(self->currentThread, (uintptr_t) thrd);
  return thrd;
}

void heap_detach_thread(struct heap* self) {
  removeThread(self, heap_get_current_thread(self));
  flup_thread_local_set(self->currentThread, (uintptr_t) NULL);
}

