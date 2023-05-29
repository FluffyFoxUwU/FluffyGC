#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include "bug.h"
#include "concurrency/rwlock.h"
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
  
  gc_free(self->gcState);
  free(self);
  gc_current = prev;
}

static struct object* doAlloc(struct managed_heap* self, struct descriptor* desc, struct heap** allocFrom) {
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
  
  struct heap_block* (*allocFunc)(struct heap*,size_t,size_t);
  allocFunc = gen->useFastOnly ? heap_alloc_fast : heap_alloc;
  block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize);
  if (block) {
    *allocFrom = gen->fromHeap;
    goto sucess;
  }
  
  gc_start(gc_current, gen, desc->objectSize);
  block = allocFunc(gen->fromHeap, desc->alignment, desc->objectSize);
  
  if (block)
    *allocFrom = gen->fromHeap;
sucess:
  return block ? &block->objMetadata : NULL;
}

struct root_ref* managed_heap_alloc_object(struct descriptor* desc) {
  context_block_gc();
  struct heap* allocFrom;
  struct root_ref* rootRef = NULL;
  struct object* obj = doAlloc(context_current->managedHeap, desc, &allocFrom);
  
  if (!obj) { 
    // Trigger Full GC
    gc_start(gc_current, NULL, SIZE_MAX);
    obj = doAlloc(context_current->managedHeap, desc, &allocFrom);
  }
  
  if (!obj)
    goto failure;
  
  rootRef = context_add_root_object(obj);
  if (!rootRef)
    heap_dealloc(allocFrom, container_of(obj, struct heap_block, objMetadata));
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
  
  list_del(&context_current->list);
  list_add(&context_current->list, &context_current->managedHeap->contextStates[CONTEXT_INACTIVE]);
  
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
  
  list_del(&context_current->list);
  list_add(&context_current->list, &context_current->managedHeap->contextStates[CONTEXT_ACTIVE]);
}

struct context* managed_heap_new_context(struct managed_heap* self) {
  struct gc_struct* prev = gc_current;
  gc_current = self->gcState;
  
  struct context* ctx = context_new();
  ctx->managedHeap = self;
  list_add(&ctx->list, &self->contextStates[CONTEXT_INACTIVE]);
  
  gc_current = prev;
  return ctx;
}

void managed_heap_free_context(struct managed_heap* self, struct context* ctx) {
  BUG_ON(ctx->managedHeap != self);
  struct context* prev = context_current;
  context_current = ctx;
  gc_current = context_current->managedHeap->gcState;
  
  list_del(&ctx->list);
  context_free(ctx);
  
  context_current = prev;
  gc_current = context_current ? context_current->managedHeap->gcState : NULL;
}
