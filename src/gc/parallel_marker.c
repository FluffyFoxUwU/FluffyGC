#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "parallel_marker.h"

#include "gc.h"
#include "root.h"
#include "heap.h"
#include "region.h"
#include "root_iterator.h"
#include "marker.h"
#include "cardtable_iterator.h"
#include "thread_pool.h"
#include "config.h"

void gc_parallel_marker(struct gc_state* gcState, bool isYoung) {
  struct region* markRegion = gcState->heap->oldGeneration;
  struct object_info* cardTableObjects = gcState->heap->youngObjects;
  atomic_bool* cardTable = gcState->heap->youngToOldCardTable;
  struct region* cardtableRegion = gcState->heap->youngGeneration;
  size_t cardTableSize = gcState->heap->youngToOldCardTableSize;
  
  if (isYoung) {
    cardtableRegion = gcState->heap->oldGeneration;
    markRegion = gcState->heap->youngGeneration;
    cardTable = gcState->heap->oldToYoungCardTable;
    cardTableObjects = gcState->heap->oldObjects;
    cardTableSize = gcState->heap->oldToYoungCardTableSize;
  }
  
  // Each worker thread
  __block gc_mark_executor executor;
  __block thread_pool_worker workerTask = ^void (struct thread_pool_work_unit work) {
    __block int workCount = 0;

    struct gc_marker_args args = *((struct gc_marker_args*) work.data);
    args = gc_marker_builder()
      ->copy_from(args)
      ->executor(^void (struct gc_marker_args args) {
        if (workCount < CONFIG_GC_MARK_WORK_SIZE)
          return gc_marker_mark(args);

        workCount = 0;
        executor(args);
      })
      ->build();
    free(work.data);
    gc_marker_mark(args);
  };

  executor = ^void (struct gc_marker_args args) {
    struct thread_pool_work_unit workUnit = {
      .worker = workerTask,
      .canRelease = false
    };
    workUnit.data = malloc(sizeof(struct gc_marker_args));
    assert(workUnit.data);
    *((struct gc_marker_args*) workUnit.data) = args;

    if (!thread_pool_try_submit_work(gcState->workerPool, &workUnit))
      workerTask(workUnit);
  };
  
  struct gc_marker_args defaultArgs = gc_marker_builder()
            ->gc_state(gcState)
            ->ignore_weak(true)
            ->ignore_soft(gcState->isExplicit)
            ->executor(executor)
            ->only_in(markRegion)
            ->build();
  
  // Sending jobs
  root_iterator_run(root_iterator_builder()
      ->ignore_weak(true)
      ->heap(gcState->heap)
      ->only_in(markRegion)
      ->consumer(^void (struct root_reference* ref, struct object_info* info) {
        atomic_fetch_add(&info->strongRefCount, 1);
        executor(gc_marker_builder()
            ->copy_from(defaultArgs)
            ->object_info(info)
            ->build()
        );
      })
      ->build()
  ); 
  
  cardtable_iterator_do(gcState->heap, cardTableObjects, cardTable, cardTableSize, ^void (struct object_info* info, int cardTableIndex) {
    executor(gc_marker_builder()
        ->copy_from(defaultArgs)
        ->object_info(info)
        ->build()
    );
  });
   
  thread_pool_wait_all(gcState->workerPool);
}

