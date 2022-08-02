#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <Block.h>
#include <threads.h>

#include "region.h"
#include "root_iterator.h"
#include "heap.h"
#include "thread.h"
#include "root.h"
#include "config.h"
#include "builder.h"

static void iterateRoot(struct root* root, struct root_iterator_args* ctx) {
  for (int i = 0; i < root->size; i++) {
    struct root_reference* rootRef = root->entries[i].refToSelf;

    if (!rootRef || !rootRef->isValid)
      continue;

    if (rootRef->isWeak && rootRef->data)
      continue;
    
    struct region_reference* ref = (struct region_reference*) rootRef->data;
    assert(ref);

    if (ctx->onlyIn && ref->owner != ctx->onlyIn)
      continue;
    if (ctx->ignoreWeak && rootRef->isWeak)
      continue;

    ctx->consumer(rootRef, heap_get_object_info(ctx->heap, ref));
  }
}

static void iterateThread(struct thread* thread, struct root_iterator_args* ctx) {
  // Iterate each frame
  for (int i = 0; i < thread->topFramePointer; i++)
    if (thread->frames[i].isValid)
      iterateRoot(thread->frames[i].root, ctx);
}

void root_iterator_run(struct root_iterator_args args) {
  // Iterate common roots (global root)
  pthread_rwlock_rdlock(&args.heap->globalRootRWLock);
  iterateRoot(args.heap->globalRoot, &args);
  pthread_rwlock_unlock(&args.heap->globalRootRWLock);

  // Iterate each threads
  for (int i = 0; i < args.heap->threadsListSize; i++)
    if (args.heap->threads[i])
      iterateThread(args.heap->threads[i], &args);
}


// Its completely fine as long
// you dont reuse builders
static thread_local struct root_iterator_builder_struct builder = {
  .data = {},

  BUILDER_SETTER(builder, struct heap*, heap, heap),
  BUILDER_SETTER(builder, root_iterator_consumer_t, consumer, consumer),
  BUILDER_SETTER(builder, struct region*, onlyIn, only_in),
  BUILDER_SETTER(builder, bool, ignoreWeak, ignore_weak),

  .build = ^struct root_iterator_args () {
    return builder.data;
  }
};

struct root_iterator_builder_struct* root_iterator_builder() {
  struct root_iterator_args tmp = {};
  builder.data = tmp;
  return &builder;
}






