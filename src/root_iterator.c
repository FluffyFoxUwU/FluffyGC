#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "region.h"
#include "root_iterator.h"
#include "reference.h"
#include "heap.h"
#include "thread.h"
#include "root.h"

struct context {
  root_iterator2_t iterator;
  struct region* onlyIn;
  struct heap* heap;
};

static void iterateRoot(struct root* root, struct context* ctx) {
  for (int i = 0; i < root->size; i++) {
    if (!root->entries[i].isValid)
      continue;
    
    struct region_reference* ref = (struct region_reference*) root->entries[i].data;
    assert(ref);

    if (ctx->onlyIn && ref->owner != ctx->onlyIn)
      continue;

    ctx->iterator(&root->entries[i], heap_get_object_info(ctx->heap, ref));
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






