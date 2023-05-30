#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bug.h"
#include "gc/gc.h"
#include "managed_heap.h"
#include "memory/heap.h"
#include "memory/soc.h"
#include "context.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "util/btree.h"
#include "util/util.h"

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

static struct descriptor_type type = {
  .id.name = "test_object",
  .id.ownerID = 0x00,
  .id.typeID = 0x00,
  .objectSize = sizeof(struct test_object),
  .alignment = alignof(struct test_object),
  .fields = {
    DESCRIPTOR_FIELD(struct test_object, next, OBJECT_NORMAL, REFERENCE_STRONG),
    DESCRIPTOR_FIELD(struct test_object, prev, OBJECT_NORMAL, REFERENCE_STRONG),
    DESCRIPTOR_FIELD_END()
  }
};

struct descriptor* desc;
struct managed_heap* heap;
static void* worker(void*) {
  managed_heap_attach_context(heap);
  
  while (true) {
    struct root_ref* obj = managed_heap_alloc_object(desc);
    if (!obj) {
      printf("[Main] Heap OOM-ed\n");
      BUG();
    }
    context_remove_root_object(obj);
  }
  
  managed_heap_detach_context(heap);
}

int main2(int argc, char** argv) {
  printf("Hello World!\n");
  
  // struct btree tree;
  // btree_init(&tree);
  // btree_add_range(&tree, &(struct btree_range) {3, 5}, (void*) 0xDEADBEE0);
  // btree_cleanup(&tree);
  
  struct generation_params params[] = {
    {
      .size = 4 * 1024 * 1024,
      .earlyPromoteSize = 4 * 1024
    }
  };
  
  desc = descriptor_new(&type);
  heap = managed_heap_new(GC_NOP_GC, ARRAY_SIZE(params), params, 0);
  BUG_ON(!heap);
  
  pthread_t threads[2] = {};
  for (int i = 0; i < ARRAY_SIZE(threads); i++)
    pthread_create(&threads[i], NULL, worker, NULL);
  for (int i = 0; i < ARRAY_SIZE(threads); i++)
    pthread_join(threads[i], NULL);
  
  managed_heap_free(heap);
  descriptor_release(desc);
  return EXIT_SUCCESS;
}

static void* worker2(void* _udata) {
  gc_current = gc_new(GC_NOP_GC, 0);
  context_current = context_new();
  
  struct heap* heap = _udata;
  for (int i = 0; i < 1'000'000; i++) {
    struct heap_block* blk = heap_alloc(heap, alignof(struct test_object), sizeof(struct test_object));
    BUG_ON(!blk);
    heap_dealloc(heap, blk);
  }
  
  context_free(context_current);
  gc_free(gc_current);
  return NULL;
}

int main32(int argc, char** argv) {
  struct heap* heap = heap_new(8 * 1024 * 1024);
  
  pthread_t threads[2] = {};
  for (int i = 0; i < ARRAY_SIZE(threads); i++)
    pthread_create(&threads[i], NULL, worker2, heap);
  for (int i = 0; i < ARRAY_SIZE(threads); i++)
    pthread_join(threads[i], NULL);
  
  heap_free(heap);
  return EXIT_SUCCESS;
}
