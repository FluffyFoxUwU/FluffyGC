#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include "bug.h"
#include "gc/gc.h"
#include "gc/gc_flags.h"
#include "gc/gc_statistic.h"
#include "managed_heap.h"
#include "memory/heap.h"
#include "context.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "util/util.h"
#include "vec.h"

/*
layout of object
struct object {
  struct object* forwardingPointer;
  struct descriptor* descriptor;
  size_t objectSize; // Represent size of data in payload (including redzones)
  int age; // Number of collection survived
  
  char payload[];
}

actual writable address is `obj->payload + obj->redzoneSize` to 
account redzones which are implemented using ASAN poisoning facility
*/

struct test_object {
  _Atomic(struct test_object*) next;
  _Atomic(struct test_object*) prev;
  
  long uwu;
  void* owo;
  int fur;
  double furry;
  int data;
};

struct descriptor* testObjectClass;
struct managed_heap* heap;
static void* worker(void*) {
  managed_heap_attach_context(heap);
  util_set_thread_name("FlGC: Worker");
  
  struct root_ref* obj = managed_heap_alloc_object(testObjectClass);
  if (!obj) {
    printf("[Main] Heap OOM-ed\n");
    BUG();
  }
  
  for (int i = 0; i >= 0; i++) {
    struct root_ref* obj3 = managed_heap_alloc_object(testObjectClass);
    if (!obj3) {
      printf("[Main] Heap OOM-ed\n");
      BUG();
    }
    for (int i = 0; i <= 2'000; i++) {
      struct root_ref* obj2 = managed_heap_alloc_object(testObjectClass);
      if (!obj2) {
        printf("[Main] Heap OOM-ed\n");
        BUG();
      }
      
      context_block_gc();
      //object_write_reference(atomic_load(&obj2->obj), offsetof(struct test_object, next), atomic_load(&obj->obj));
      //swap(obj2, obj);
      context_unblock_gc();
      
      context_remove_root_object(obj2);
    }
    context_remove_root_object(obj3);
  }
  
  managed_heap_detach_context(heap);
  
  return NULL;
}

static atomic_bool shutDownFlag;
static void* statPrinter(void*) {
  managed_heap_attach_context(heap);
  
  util_set_thread_name("FlGC: Stat printer");
  
  struct gc_statistic prev = {};
  while (atomic_load(&shutDownFlag) == false) {
    context_block_gc();
    printf("[Heap] ");
    for (int i = 0; i < managed_heap_current->generationCount; i++) {
      struct generation* gen = &managed_heap_current->generations[i];
      printf("Gen%d: %8zu bytes / %8zu bytes   ", i, gen->fromHeap->usage, gen->fromHeap->size);
    }
    puts("");
    
    struct gc_statistic updated = gc_current->stat;
    struct gc_statistic diff = {
      .promotedBytes = updated.promotedBytes - prev.promotedBytes,
      .reclaimedBytes = updated.reclaimedBytes - prev.reclaimedBytes,
      .promotedObjects = updated.promotedObjects - prev.promotedObjects,
    };
    printf("[GC] Promoted  %15zu bytes/s\n", diff.promotedBytes);
    printf("[GC] Reclaimed %15zu MiB/s\n", diff.reclaimedBytes / 1024 / 1024);
    printf("[GC] Promoted  %15" PRIu64 " objects/s\n", diff.promotedObjects);
    prev = gc_current->stat;
    context_unblock_gc();
    
    sleep(1);
  }
  
  managed_heap_detach_context(heap);
  return NULL;
}

int main2(int argc, char** argv) {
  printf("Hello World!\n");
  
  // struct btree tree;
  // btree_init(&tree);
  // btree_add_range(&tree, &(struct btree_range) {3, 5}, (void*) 0xDEADBEE0);
  // btree_cleanup(&tree);
  
  gc_flags gcFlags = SERIAL_GC_USE_3_GENERATIONS;
  struct generation_params params[] = {
    {
      .size = 1 * 1024 * 1024,
      .earlyPromoteSize = 4 * 1024,
      .promotionAge = 0
    },
    {
      .size = 8 * 1024 * 1024,
      .earlyPromoteSize = 8 * 1024,
      .promotionAge = 0
    },
    {
      .size = 16 * 1024 * 1024,
      .earlyPromoteSize = -1,
      .promotionAge = -1
    }
  };
  
  testObjectClass = descriptor_new();
  
  struct descriptor_field typeFields[] = {
    DESCRIPTOR_FIELD(struct test_object, next, testObjectClass, REFERENCE_STRONG),
    DESCRIPTOR_FIELD(struct test_object, prev, testObjectClass, REFERENCE_STRONG),
    DESCRIPTOR_FIELD_END()
  };
  testObjectClass->name = strdup("test_object");
  testObjectClass->objectSize = sizeof(struct test_object);
  testObjectClass->alignment = alignof(struct test_object);
  testObjectClass->objectType = OBJECT_NORMAL;
  
  for (int i = 0; i < ARRAY_SIZE(typeFields); i++)
    vec_push(&testObjectClass->fields, typeFields[i]);
  
  descriptor_init(testObjectClass);
  
  heap = managed_heap_new(GC_SERIAL_GC, 3, params, gcFlags);
  BUG_ON(!heap);
  
  atomic_init(&shutDownFlag, false);
  
  pthread_t threads[2] = {};
  pthread_t statPrinterThread;
  pthread_create(&statPrinterThread, NULL, statPrinter, NULL);
  
  for (int i = 0; i < ARRAY_SIZE(threads); i++)
    pthread_create(&threads[i], NULL, worker, NULL);
  for (int i = 0; i < ARRAY_SIZE(threads); i++)
    pthread_join(threads[i], NULL);
  
  atomic_store(&shutDownFlag, true);
  pthread_join(statPrinterThread, NULL);
  
  managed_heap_free(heap);
  descriptor_release(testObjectClass);
  return EXIT_SUCCESS;
}
