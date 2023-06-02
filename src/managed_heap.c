#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <threads.h>

#include "bug.h"
#include "concurrency/completion.h"
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

thread_local struct managed_heap* managed_heap_current;

static void freeGeneration(struct managed_heap* self, struct generation* gen) {
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &gen->rememberedSet)
    list_del(current);
  mutex_cleanup(&gen->rememberedSetLock);
  heap_free(gen->fromHeap);
  heap_free(gen->toHeap);
}

static int initGeneration(struct managed_heap* self, struct generation* gen, struct generation_params* param, bool useFastOnly) {
  if (mutex_init(&gen->rememberedSetLock) < 0)
    goto failure;
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

struct managed_heap* managed_heap_new(enum gc_algorithm algo, int generationCount, struct generation_params* genParam, gc_flags gcFlags) {
  if (generationCount > GC_MAX_GENERATIONS || gc_generation_count(algo, gcFlags) != generationCount)
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
      completion_init(&self->gcCompleted) < 0)
    goto failure;
  
  if (!(self->gcState = gc_new(algo, gcFlags)))
    goto failure;
  
  for (int i = 0; i < generationCount; i++) {
    if (initGeneration(self, &self->generations[i], &genParam[i], gc_use_fast_on_gen(algo, gcFlags, i)) < 0)
      goto failure;
    self->generations[i].genID = i;
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
  completion_cleanup(&self->gcCompleted);
  gc_free(self->gcState);
  free(self);
  gc_current = prev;
}

static struct heap_block* doAlloc(struct managed_heap* self, struct descriptor* desc, struct generation** attemptedOn) {
  struct heap_block* block = NULL;
  struct generation* gen = NULL;
  for (int i = 0; i < self->generationCount; i++) {
    struct generation* tmp = &self->generations[i];
    if (i + 1 >= self->generationCount || desc->objectSize < tmp->param.earlyPromoteSize) {
      gen = tmp;
      break;
    }
  }
  
  // No generation big enough
  if (!gen)
    return NULL;
  *attemptedOn = gen;
  
  struct heap_block* (*allocFunc)(struct heap*,size_t,size_t);
  allocFunc = gen->useFastOnly ? heap_alloc_fast : heap_alloc;
  
  if ((block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize)))
    goto allocSuccess;
    
  if (gc_upgrade_to_gc_mode(gc_current)) {
    mutex_lock(&self->gcCompleted.onComplete.lock);
    completion_reset(&self->gcCompleted);
    mutex_unlock(&self->gcCompleted.onComplete.lock);
    
    // This thread is the winner
    // and this thread got first attempt to allocate
    
    gc_start(self->gcState, gen);
    if ((block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize))) { 
      complete_all(&self->gcCompleted);
      goto allocSuccess;
    }
    
    gc_start(self->gcState, NULL);
    block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize);
    
    complete_all(&self->gcCompleted);
    gc_downgrade_from_gc_mode(gc_current);
  } else {
    // Concurrent thread tried to start GC
    // so make the rest waits then retry
    // so be sure it wont spam GC cycles with no benefit
    
    gc_unblock(gc_current);
    wait_for_completion(&self->gcCompleted);
    gc_block(gc_current);
    block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize);
  }
  
allocSuccess:
  *attemptedOn = gen;
  return block;
}

struct root_ref* managed_heap_alloc_object(struct descriptor* desc) {
  context_block_gc();
  struct generation* attemptedOn;
  struct root_ref* rootRef = NULL;
  struct heap_block* newBlock = doAlloc(managed_heap_current, desc, &attemptedOn);
  
  if (!newBlock)
    goto failure;
  
  rootRef = context_add_root_object(&newBlock->objMetadata);
  if (!rootRef && newBlock) {
    heap_dealloc(attemptedOn->fromHeap, newBlock);
    goto failure;
  }
  object_init(&newBlock->objMetadata, desc, newBlock->dataPtr.ptr);
  BUG_ON(!attemptedOn);
  newBlock->objMetadata.generationID = attemptedOn->genID;
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
  BUG_ON(!context_current || context_current->managedHeapa != self);
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
  
  mutex_lock(&managed_heap_current->contextTrackerLock);
  list_del(&context_current->list);
  list_add(&context_current->list, &managed_heap_current->contextStates[CONTEXT_INACTIVE]);
  mutex_unlock(&managed_heap_current->contextTrackerLock);
  
  context_current = NULL;
  gc_current = NULL;
  managed_heap_current = NULL;
  return current;
}

void managed_heap_switch_context_in(struct context* new) {
  BUG_ON(new->state != CONTEXT_INACTIVE);
  BUG_ON(context_current);
  
  managed_heap_current = new->managedHeapa;
  context_current = new;
  gc_current = managed_heap_current->gcState;
  context_current->state = CONTEXT_ACTIVE;
  
  mutex_lock(&managed_heap_current->contextTrackerLock);
  list_del(&context_current->list);
  list_add(&context_current->list, &managed_heap_current->contextStates[CONTEXT_ACTIVE]);
  mutex_unlock(&managed_heap_current->contextTrackerLock);
}

struct context* managed_heap_new_context(struct managed_heap* self) {
  struct gc_struct* prev = gc_current;
  gc_current = self->gcState;
  
  struct context* ctx = context_new();
  if (!ctx)
    goto failure;
  
  ctx->managedHeapa = self;
  mutex_lock(&self->contextTrackerLock);
  list_add(&ctx->list, &self->contextStates[CONTEXT_INACTIVE]);
  mutex_unlock(&self->contextTrackerLock);
  
failure:
  gc_current = prev;
  return ctx;
}

void managed_heap_free_context(struct managed_heap* self, struct context* ctx) {
  BUG_ON(ctx->managedHeapa != self);
  struct context* prev = context_current;
  context_current = ctx;
  gc_current = managed_heap_current->gcState;
  
  mutex_lock(&managed_heap_current->contextTrackerLock);
  list_del(&ctx->list);
  mutex_unlock(&managed_heap_current->contextTrackerLock);
  context_free(ctx);
  
  context_current = prev;
}
