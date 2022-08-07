#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "heap.h"
#include "parallel_heap_iterator.h"

#include "gc.h"
#include "region.h"
#include "thread_pool.h"
#include "config.h"
#include "util.h"

bool gc_parallel_heap_iterator_do(struct gc_state* gcState, bool isYoung, gc_parallel_heap_iterator consumer, int* lastPos) {
  struct region* iterateRegion = gcState->heap->oldGeneration;
  struct object_info* objects = gcState->heap->oldObjects;
  
  int tmp = 0;
  if (!lastPos)
    lastPos = &tmp;

  if (isYoung) {
    iterateRegion = gcState->heap->youngGeneration;
    objects = gcState->heap->youngObjects;
  }

  __block atomic_int failedPos;
  __block atomic_int currentPos;

  atomic_init(&currentPos, 0);
  atomic_init(&failedPos, iterateRegion->sizeInCells);
  
  thread_pool_worker worker = ^void (struct thread_pool_work_unit workUnit) {
    int i = 0;
    while ((i = atomic_fetch_add(&currentPos, 1)) < iterateRegion->sizeInCells) {
      bool res = consumer(&objects[i], i);
      if (!res) {
        util_atomic_min_int(&failedPos, i);
        break;
      }
    } 
  };
 
  struct thread_pool_work_unit work = {
    .worker = worker,
    .canRelease = false
  };
  
  for (int i = 0; i < gcState->workerPool->poolSize; i++)
    thread_pool_submit_work(gcState->workerPool, &work);
  thread_pool_wait_all(gcState->workerPool);
   
  *lastPos = atomic_load(&currentPos);
  if (atomic_load(&failedPos) < iterateRegion->sizeInCells)
    *lastPos = atomic_load(&failedPos);
  
  return atomic_load(&failedPos) == iterateRegion->sizeInCells;
}




