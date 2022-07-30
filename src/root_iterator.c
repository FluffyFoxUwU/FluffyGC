#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "region.h"
#include "root_iterator.h"
#include "heap.h"
#include "thread.h"
#include "root.h"
#include "config.h"

struct context {
  root_iterator2_t iterator;
  struct region* onlyIn;
  struct heap* heap;
};

static void iterateRoot(struct root* root, struct context* ctx) {
  for (int i = 0; i < root->size; i++) {
    struct root_reference* rootRef = root->entries[i].refToSelf;

    if (!rootRef || !rootRef->isValid)
      continue;
    
    struct region_reference* ref = (struct region_reference*) rootRef->data;
    assert(ref);

    if (ctx->onlyIn && ref->owner != ctx->onlyIn)
      continue;

    ctx->iterator(rootRef, heap_get_object_info(ctx->heap, ref));
  }
}

static void iterateThread(struct thread* thread, struct context* ctx) {
  // Iterate each frame
  for (int i = 0; i < thread->framePointer; i++)
    if (thread->frames[i].isValid)
      iterateRoot(thread->frames[i].root, ctx);
}

void root_iterator_run2(struct heap* heap, struct region* onlyIn, root_iterator2_t iterator) {
  struct context ctx = {
    .iterator = iterator,
    .onlyIn = onlyIn,
    .heap = heap
  };

  // Iterate common roots (global root)
  pthread_rwlock_rdlock(&heap->globalRootRWLock);
  iterateRoot(heap->globalRoot, &ctx);
  pthread_rwlock_unlock(&heap->globalRootRWLock);

  // Iterate each threads
  for (int i = 0; i < heap->threadsListSize; i++)
    if (heap->threads[i])
      iterateThread(heap->threads[i], &ctx);
}

void root_iterator_run(struct heap* heap, struct region* onlyIn, root_iterator_t iterator) {
  root_iterator_run2(heap, onlyIn, ^void (struct root_reference* ref, struct object_info* obj) {
    iterator(obj);
  });
}






