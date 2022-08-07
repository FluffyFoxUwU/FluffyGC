#include <stdatomic.h>

#include "cardtable_iterator.h"
#include "../config.h"
#include "../heap.h"
#include "gc.h"
#include "thread_pool.h"

void cardtable_iterator_do(struct heap* heap, struct object_info* objects, atomic_bool* cardTable, size_t cardTableSize, cardtable_iterator iterator) {
  __block atomic_int current;
  atomic_init(&current, 0);

  struct thread_pool_work_unit work = {
    .worker = ^void (const struct thread_pool_work_unit* work) {
      int i = 0;
      while ((i = atomic_fetch_add(&current, 1)) < cardTableSize) {  
        if (atomic_load(&cardTable[i]) == false)
          continue;

        int rangeStart = i * CONFIG_CARD_TABLE_PER_BUCKET_SIZE;

        bool encounterAny = false;    
        for (int j = 0; j < CONFIG_CARD_TABLE_PER_BUCKET_SIZE; j++) {
          struct object_info* info = &objects[rangeStart + j];
           
          if (!info->regionRef || info->isValid == false)
            continue;

          iterator(info, i);
          encounterAny = true;
        }

        if (!encounterAny)
          atomic_store(&cardTable[i], false);
      }
    },
    .canRelease = false
  };
  
  for (int i = 0; i < heap->gcState->workerPool->poolSize; i++)
    thread_pool_submit_work(heap->gcState->workerPool, &work);
  thread_pool_wait_all(heap->gcState->workerPool);
}

