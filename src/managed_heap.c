#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <threads.h>

#include "api/api.h"
#include "api/type_registry.h"
#include "bug.h"
#include "concurrency/completion.h"
#include "concurrency/mutex.h"
#include "hook/hook.h"
#include "managed_heap.h"
#include "context.h"
#include "gc/gc.h"
#include "memory/heap.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "util/id_generator.h"
#include "util/list_head.h"

thread_local struct managed_heap* managed_heap_current;

static void freeGeneration(struct generation* gen) {
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &gen->rememberedSet)
    list_del(current);
  mutex_cleanup(&gen->rememberedSetLock);
  heap_free(gen->fromHeap);
  heap_free(gen->toHeap);
}

static int initGeneration(struct generation* gen, struct generation_params* param, bool useFastOnly) {
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
  freeGeneration(gen);
  return -ENOMEM;
}

struct managed_heap* managed_heap_new(enum gc_algorithm algo, int generationCount, struct generation_params* genParam, gc_flags gcFlags) {
  if (generationCount > GC_MAX_GENERATIONS || gc_generation_count(algo, gcFlags) != generationCount)
    return NULL;
   
  int hookRet = hook_init();
  if (hookRet < 0 && hookRet != -ENOSYS)
    return NULL;
  
  struct managed_heap* self = malloc(sizeof(*self) + sizeof(*self->generations) * generationCount);
  if (!self)
    return NULL;
  
  *self = (struct managed_heap) {
    .generationCount = 0,
    .idGenerators = {
      .object = ID_GENERATOR_INIT,
      .context = ID_GENERATOR_INIT,
      .dmaPtr = ID_GENERATOR_INIT,
      .descriptor = ID_GENERATOR_INIT
    }
  };
  
  for (int i = 0; i < CONTEXT_STATE_COUNT; i++)
    list_head_init(&self->contextStates[i]);
  list_head_init(&self->descriptorList);
  
  if (mutex_init(&self->contextTrackerLock) < 0 ||
      completion_init(&self->gcCompleted) < 0)
    goto failure;
  
  if (!(self->api.registry = type_registry_new()))
    goto failure;
  
  int ret = 0;
  managed_heap_push_states(self, NULL);
  ret = api_init();
  managed_heap_pop_states();
  if (ret < 0)
    goto failure;
  
  for (int i = 0; i < generationCount; i++) {
    if (initGeneration(&self->generations[i], &genParam[i], gc_use_fast_on_gen(algo, gcFlags, i)) < 0)
      goto failure;
    self->generations[i].genID = i;
    self->generationCount++;
  }
  
  // GC is heavier to initialize, make sure its 
  // placed at the last
  managed_heap_push_states(self, NULL);
  self->gcState = gc_new(algo, gcFlags);
  managed_heap_pop_states();
  if (!self->gcState)
    goto failure;
  
  static struct id_generator_state generator = ID_GENERATOR_INIT;
  self->foreverUniqueID = id_generator_get(&generator);
  return self;
failure:
  managed_heap_free(self);
  return NULL;
}

void managed_heap_free(struct managed_heap* self) {
  if (!self)
    return;
  
  BUG_ON(managed_heap_current || context_current || gc_current);
  
  managed_heap_push_states(self, NULL);
  
  mutex_lock(&self->contextTrackerLock);
  // There is contexts!!
  BUG_ON(self->contextCount > 0);
  mutex_unlock(&self->contextTrackerLock);
  
  gc_free(self->gcState);
  
  for (int i = 0; i < self->generationCount; i++)
    freeGeneration(&self->generations[i]);
  
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->descriptorList)
    descriptor_free(list_entry(current, struct descriptor, list));
  
  api_cleanup();
  
  type_registry_free(self->api.registry);
  mutex_cleanup(&self->contextTrackerLock);
  completion_cleanup(&self->gcCompleted);
  free(self);
  
  managed_heap_pop_states();
}

static struct heap_block* doAlloc(struct managed_heap* self, struct descriptor* desc, struct generation** attemptedOn) {
  struct heap_block* block = NULL;
  struct generation* gen = NULL;
  for (int i = 0; i < self->generationCount; i++) {
    struct generation* tmp = &self->generations[i];
    if (i + 1 >= self->generationCount || descriptor_get_object_size(desc) < tmp->param.earlyPromoteSize) {
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
  
  if ((block = allocFunc(gen->fromHeap, descriptor_get_alignment(desc), descriptor_get_object_size(desc))))
    goto allocSuccess;
  
  while (!block) {
    if (gc_upgrade_to_gc_mode(gc_current)) {
      mutex_lock(&self->gcCompleted.lock);
      completion_reset(&self->gcCompleted);
      mutex_unlock(&self->gcCompleted.lock);
      
      // This thread is the winner
      // and this thread got exclusive attempt to allocate
      
      gc_start(self->gcState, gen);
      block = allocFunc(gen->fromHeap, descriptor_get_alignment(desc), descriptor_get_object_size(desc));
      complete_all(&self->gcCompleted);
      if (block)
        break;
      
      mutex_lock(&self->gcCompleted.lock);
      completion_reset(&self->gcCompleted);
      mutex_unlock(&self->gcCompleted.lock);
      
      gc_start(self->gcState, NULL);
      block = allocFunc(gen->fromHeap, descriptor_get_alignment(desc), descriptor_get_object_size(desc));
      complete_all(&self->gcCompleted);
      if (!block)
        break;
      
      gc_downgrade_from_gc_mode(gc_current);
    } else {
      // Concurrent thread tried to start GC
      // so make the rest waits then retry
      // so be sure it wont spam GC cycles with no benefit
      
      gc_unblock(gc_current);
      wait_for_completion(&self->gcCompleted);
      gc_block(gc_current);
      
      if ((block = allocFunc(gen->fromHeap, descriptor_get_alignment(desc), descriptor_get_object_size(desc))))
        break;
    }
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
  
  gc_current->ops->postObjectAlloc(&newBlock->objMetadata);
  
  rootRef = context_add_root_object(&newBlock->objMetadata);
  if (!rootRef && newBlock) {
    heap_dealloc(attemptedOn->fromHeap, newBlock);
    goto failure;
  }
  object_init(&newBlock->objMetadata, desc, newBlock->dataPtr.ptr);
  BUG_ON(!attemptedOn);
  newBlock->objMetadata.movePreserve.generationID = attemptedOn->genID;
failure:
  context_unblock_gc();
  return rootRef;
}

static void managed_heap_set_context_state_nolock(struct context* context, enum context_state state) {
  context->state = state;
  list_del(&context_current->list);
  list_add(&context_current->list, &managed_heap_current->contextStates[state]);
}

void managed_heap_set_context_state(struct context* context, enum context_state state) {
  if (context_current)
    context_block_gc();
  else
    gc_block(gc_current);
  
  mutex_lock(&managed_heap_current->contextTrackerLock);
  managed_heap_set_context_state_nolock(context, state);
  mutex_unlock(&managed_heap_current->contextTrackerLock);
  
  if (context_current)
    context_unblock_gc();
  else
    gc_unblock(gc_current);
}

static struct context* switchContextOutNolock() {
  BUG_ON(!context_current);
  
  struct context* current = context_current;
  managed_heap_set_context_state_nolock(current, CONTEXT_INACTIVE);
  
  managed_heap_set_states(NULL, NULL);
  return current;
}

static int switchContextInNolock(struct context* new) {
  BUG_ON(context_current);
  
  int res = 0;
  if (new->state != CONTEXT_INACTIVE) {
    res = -EBUSY;
    goto context_in_use;
  }
  
  managed_heap_set_states(new->managedHeap, new);
  managed_heap_set_context_state_nolock(new, CONTEXT_RUNNING);
context_in_use:
  return res;
}

int managed_heap_attach_thread(struct managed_heap* self) {
  BUG_ON(context_current);
  
  managed_heap_set_states(self, NULL);
  struct context* ctx = managed_heap_new_context(self);
  if (!ctx)
    return -ENOMEM;
  
  bool doRawBlock = context_current == NULL;
  if (doRawBlock)
    gc_block(gc_current);
  else
    context_block_gc();
  
  mutex_lock(&self->contextTrackerLock);
  switchContextInNolock(ctx);
  mutex_unlock(&self->contextTrackerLock);
  
  if (doRawBlock)
    gc_unblock(gc_current);
  else
    context_unblock_gc();
  return 0;
}

void managed_heap_detach_thread(struct managed_heap* self) {
  BUG_ON(!context_current || context_current->managedHeap != self);
  managed_heap_free_context(self, context_current);
  managed_heap_set_states(NULL, NULL);
}

int managed_heap_swap_context(struct context* new) {
  BUG_ON(new->managedHeap != context_current->managedHeap);
  if (context_current)
    context_block_gc();
  else
    gc_block(gc_current);
  
  struct managed_heap* heap = managed_heap_current;
  mutex_lock(&heap->contextTrackerLock);
  int ret = 0;
  struct context* old = switchContextOutNolock();
  ret = switchContextInNolock(new);
  
  // Restore context
  if (ret < 0) {
    int ret2 = switchContextInNolock(old);
    BUG_ON(ret2 != 0);
  }
  
  mutex_unlock(&heap->contextTrackerLock);
  if (context_current)
    context_unblock_gc();
  else
    gc_unblock(gc_current);
  return ret;
}

struct context* managed_heap_new_context(struct managed_heap* self) {
  struct context* ctx;
  managed_heap_push_states(self, NULL);
  ctx = context_new(self);
  managed_heap_pop_states();
  if (!ctx)
    goto failure;
  
  ctx->state = CONTEXT_INACTIVE;
  
  mutex_lock(&self->contextTrackerLock);
  self->contextCount++;
  list_add(&ctx->list, &self->contextStates[CONTEXT_INACTIVE]);
  mutex_unlock(&self->contextTrackerLock);
  
failure:
  return ctx;
}

void managed_heap_free_context(struct managed_heap* self, struct context* ctx) {
  BUG_ON(ctx->managedHeap != self);
  managed_heap_push_states(self, ctx);
  if (context_current)
    context_block_gc();
  else
    gc_block(gc_current);
  
  mutex_lock(&managed_heap_current->contextTrackerLock);
  self->contextCount--;
  list_del(&ctx->list);
  mutex_unlock(&managed_heap_current->contextTrackerLock);
  
  if (context_current)
    context_unblock_gc();
  else
    gc_unblock(gc_current);
  
  context_free(ctx);
  managed_heap_pop_states();
}

uint64_t managed_heap_generate_object_id() {
  if (managed_heap_current)
    return id_generator_get(&managed_heap_current->idGenerators.object);
  return 1;
}

uint64_t managed_heap_generate_context_id() {
  if (managed_heap_current)
    return id_generator_get(&managed_heap_current->idGenerators.context);
  return 1;
}

uint64_t managed_heap_generate_dma_ptr_id() {
  if (managed_heap_current)
    return id_generator_get(&managed_heap_current->idGenerators.dmaPtr);
  return 1;
}

uint64_t managed_heap_generate_descriptor_id() {
  if (managed_heap_current)
    return id_generator_get(&managed_heap_current->idGenerators.descriptor);
  return 1;
}
