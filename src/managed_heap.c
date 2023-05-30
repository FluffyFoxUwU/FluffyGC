#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <errno.h>

#include "bug.h"
#include "concurrency/event.h"
#include "concurrency/mutex.h"
#include "concurrency/rwlock.h"
#include "concurrency/rwulock.h"
#include "managed_heap.h"
#include "context.h"
#include "gc/gc.h"
#include "memory/heap.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "util/list_head.h"
#include "util/util.h"

static void freeGeneration(struct managed_heap* self, struct generation* gen) {
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &gen->rememberedSet)
    list_del(current);
  heap_free(gen->fromHeap);
  heap_free(gen->toHeap);
}

static int initGeneration(struct managed_heap* self, struct generation* gen, struct generation_params* param, bool useFastOnly) {
  if (!(gen->fromHeap = heap_new(param->size / 2)))
    goto failure;
  if (!(gen->toHeap = heap_new(param->size / 2)))
    goto failure;
  
  list_head_init(&gen->rememberedSet);
  gen->param = *param;
  gen->useFastOnly = useFastOnly;
  return 0;
failure:
  freeGeneration(self, gen);
  return -ENOMEM;
}

struct managed_heap* managed_heap_new(enum gc_algorithm algo, int generationCount, struct generation_params* genParam, int gcFlags) {
  if (gc_generation_count(algo, gcFlags) != generationCount)
    return NULL;
  
  struct managed_heap* self = malloc(sizeof(*self) + sizeof(*self->generations) * generationCount);
  if (!self)
    return NULL;
  
  *self = (struct managed_heap) {
    .generationCount = 0
  };
  
  for (int i = 0; i < CONTEXT_STATE_COUNT; i++)
    list_head_init(&self->contextStates[i]);
  
  if (mutex_init(&self->contextTrackerLock) < 0 ||
      rwulock_init(&self->concurrentAllocationPreventer) < 0 ||
      event_init(&self->gcCompleted) < 0)
    goto failure;
  
  if (!(self->gcState = gc_new(algo, gcFlags)))
    goto failure;
  
  for (int i = 0; i < generationCount; i++) {
    if (initGeneration(self, &self->generations[i], &genParam[i], gc_use_fast_on_gen(algo, gcFlags, i)) < 0)
      goto failure;
    self->generationCount++;
  }
  return self;
failure:
  managed_heap_free(self);
  return NULL;
}

void managed_heap_free(struct managed_heap* self) {
  if (!self)
    return;
  
  struct gc_struct* prev = gc_current;
  gc_current = self->gcState;
  for (int i = 0; i < self->generationCount; i++)
    freeGeneration(self, &self->generations[i]);
  
  mutex_cleanup(&self->contextTrackerLock);
  rwulock_cleanup(&self->concurrentAllocationPreventer);
  event_cleanup(&self->gcCompleted);
  gc_free(self->gcState);
  free(self);
  gc_current = prev;
}

static struct object* doAlloc(struct managed_heap* self, struct descriptor* desc, struct generation** attemptedOn, bool isFullGC) {
  struct heap_block* block = NULL;
  struct generation* gen = NULL;
  for (int i = 0; i < self->generationCount; i++) {
    struct generation* tmp = &self->generations[i];
    if (desc->objectSize < tmp->param.earlyPromoteSize) {
      gen = tmp;
      break;
    }
  }
  
  // No generation big enough
  if (!gen)
    return NULL;
  *attemptedOn = gen;
  
  // TODO: Ensure GC can occur but after GC
  // no allocation can occur until the cause
  // of GC gets to try allocate again 
  // while fully utilizing concurrent allocation on heap allocation
  int retryCounts = 0;
  
  struct heap_block* (*allocFunc)(struct heap*,size_t,size_t);
  allocFunc = gen->useFastOnly ? heap_alloc_fast : heap_alloc;
  
  rwulock_rdlock(&self->concurrentAllocationPreventer);
retry:
  block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize);
  
  if (!block && retryCounts < MANAGED_GC_MAX_RETRIES) {
    if (rwulock_try_upgrade(&self->concurrentAllocationPreventer)) {
      // This thread is the winner
      // and this thread got first attempt to allocate
      
      mutex_lock(&self->gcCompleted.lock);
      gc_start(self->gcState, !isFullGC ? *attemptedOn : NULL, desc->objectSize);
      block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize);
      event_fire_all_nolock(&self->gcCompleted);
      mutex_unlock(&self->gcCompleted.lock);
    } else {
      // Concurrent thread tried to start GC
      // so make the rest waits then retry
      // so be sure it wont spam GC cycles with no benefit
      
      retryCounts++;
      rwulock_unlock(&self->concurrentAllocationPreventer);
      mutex_lock(&self->gcCompleted.lock);
      event_wait(&self->gcCompleted);
      mutex_unlock(&self->gcCompleted.lock);
      rwulock_rdlock(&self->concurrentAllocationPreventer);
      goto retry;
    }
  }
  rwulock_unlock(&self->concurrentAllocationPreventer);
  
  *attemptedOn = gen;
  return block ? &block->objMetadata : NULL;
}

struct root_ref* managed_heap_alloc_object(struct descriptor* desc) {
  context_block_gc();
  struct generation* attemptedOn;
  struct root_ref* rootRef = NULL;
  struct object* obj = doAlloc(context_current->managedHeap, desc, &attemptedOn, false);
  
  // Do again but with Full GC
  if (!obj)
    obj = doAlloc(context_current->managedHeap, desc, &attemptedOn, true);
  
  if (!obj)
    goto failure;
  
  rootRef = context_add_root_object(obj);
  if (!rootRef && obj)
    heap_dealloc(attemptedOn->fromHeap, container_of(obj, struct heap_block, objMetadata));
failure:
  context_unblock_gc();
  return rootRef;
}

int managed_heap_attach_context(struct managed_heap* self) {
  BUG_ON(context_current);
  
  struct context* ctx = managed_heap_new_context(self);
  if (!ctx)
    return -ENOMEM;
  managed_heap_switch_context_in(ctx);
  return 0;
}

void managed_heap_detach_context(struct managed_heap* self) {
  BUG_ON(!context_current || context_current->managedHeap != self);
  managed_heap_free_context(self, managed_heap_switch_context_out());
}

struct context* managed_heap_swap_context(struct context* new) {
  struct context* ctx = managed_heap_switch_context_out();
  managed_heap_switch_context_in(new);
  return ctx;
}

struct context* managed_heap_switch_context_out() {
  BUG_ON(!context_current);
  
  struct context* current = context_current;
  current->state = CONTEXT_INACTIVE;
  
  mutex_lock(&context_current->managedHeap->contextTrackerLock);
  list_del(&context_current->list);
  list_add(&context_current->list, &context_current->managedHeap->contextStates[CONTEXT_INACTIVE]);
  mutex_unlock(&context_current->managedHeap->contextTrackerLock);
  
  context_current = NULL;
  gc_current = NULL;
  return current;
}

void managed_heap_switch_context_in(struct context* new) {
  BUG_ON(new->state != CONTEXT_INACTIVE);
  BUG_ON(context_current);
  
  context_current = new;
  gc_current = context_current->managedHeap->gcState;
  context_current->state = CONTEXT_ACTIVE;
  
  mutex_lock(&context_current->managedHeap->contextTrackerLock);
  list_del(&context_current->list);
  list_add(&context_current->list, &context_current->managedHeap->contextStates[CONTEXT_ACTIVE]);
  mutex_unlock(&context_current->managedHeap->contextTrackerLock);
}

struct context* managed_heap_new_context(struct managed_heap* self) {
  struct gc_struct* prev = gc_current;
  gc_current = self->gcState;
  
  struct context* ctx = context_new();
  if (!ctx)
    goto failure;
  
  ctx->managedHeap = self;
  mutex_lock(&self->contextTrackerLock);
  list_add(&ctx->list, &self->contextStates[CONTEXT_INACTIVE]);
  mutex_unlock(&self->contextTrackerLock);
  
failure:
  gc_current = prev;
  return ctx;
}

void managed_heap_free_context(struct managed_heap* self, struct context* ctx) {
  BUG_ON(ctx->managedHeap != self);
  struct context* prev = context_current;
  context_current = ctx;
  gc_current = context_current->managedHeap->gcState;
  
  mutex_lock(&context_current->managedHeap->contextTrackerLock);
  list_del(&ctx->list);
  mutex_unlock(&context_current->managedHeap->contextTrackerLock);
  context_free(ctx);
  
  context_current = prev;
  gc_current = context_current ? context_current->managedHeap->gcState : NULL;
}
