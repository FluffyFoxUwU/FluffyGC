#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include "bug.h"
#include "concurrency/rwlock.h"
#include "managed_heap.h"
#include "context.h"
#include "gc/gc.h"
#include "memory/heap.h"
#include "util/list_head.h"

void managed_heap__gc_safepoint() {
  if (gc_current->hooks->onSafepoint(context_current) < 0)
    BUG();
}

void managed_heap__block_gc(struct managed_heap* self) {
  rwlock_rdlock(&self->gcLock);
}

void managed_heap__unblock_gc(struct managed_heap* self) {
  rwlock_unlock(&self->gcLock);
}

static void freeGeneration(struct managed_heap* self, struct generation* gen) {
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &gen->rememberedSet)
    list_del(current);
  heap_free(gen->fromHeap);
  heap_free(gen->toHeap);
}

static int initGeneration(struct managed_heap* self, struct generation* gen, size_t size) {
  if (!(gen->fromHeap = heap_new(size / 2)))
    goto failure;
  if (!(gen->toHeap = heap_new(size / 2)))
    goto failure;
  
  list_head_init(&gen->rememberedSet);
  return 0;
failure:
  freeGeneration(self, gen);
  return -ENOMEM;
}

struct managed_heap* managed_heap_new(enum gc_algorithm algo, int generationCount, size_t* generationSizes, int gcFlags) {
  struct managed_heap* self = malloc(sizeof(*self) + sizeof(*self->generations) * generationCount);
  if (!self)
    return NULL;
  *self = (struct managed_heap) {
    .generationCount = 0
  };
  
  rwlock_init(&self->gcLock);
  if (!(self->gcState = gc_new(algo, gcFlags)))
    goto failure;
  
  for (int i = 0; i < generationCount; i++) {
    if (initGeneration(self, &self->generations[i], generationSizes[i]) < 0)
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
  
  for (int i = 0; i < self->generationCount; i++)
    freeGeneration(self, &self->generations[i]);
  
  gc_free(self->gcState);
  rwlock_cleanup(&self->gcLock);
  free(self);
}

