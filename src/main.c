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

struct obj {
  long uwu;
  void* owo;
  int fur;
  double furry;
};

static void escape(void *p) {
  //asm volatile("" : : "g"(p) : "memory");
}

int main3(int argc, char** argv) {
  printf("Hello World!\n");
  
  struct context* context = context_new();
  context_current = context;
  
  struct heap* heap = heap_new(32 * 1024 * 1024 /* 256 * 1024 * 1024 */);
  // Set parameters
  heap_param_set_local_heap_size(heap, 4 * 1024 * 1024);
  heap_init(heap);
  
  // 16863: Just before fast alloc failed
  clock_t clk = clock();
  for (int i = 0; i <= i /* 10863 */; i++) {
    if (1) {
      struct obj* block = malloc(sizeof(struct obj));
      struct obj* block2 = malloc(sizeof(struct obj) + 1);
      escape(block);
      escape(block2);
      free(block);
      free(block2);
    } else {
      struct heap_block* block = heap_alloc(heap, alignof(struct obj), sizeof(struct obj));
      struct heap_block* block2 = heap_alloc(heap, alignof(struct obj), sizeof(struct obj) + 1);
      escape(block);
      escape(block2);
      heap_dealloc(heap, block);
      heap_dealloc(heap, block2);
    }
  }
  double timeMS = (double) (clock() - clk) / (double) CLOCKS_PER_SEC;
  printf("Time: %.2lf ms\n", timeMS);
  
  heap_merge_free_blocks(heap);
  heap_free(heap);
  context_free(context);
  return EXIT_SUCCESS;
}

struct test_object {
  _Atomic(struct test_object*) next;
  _Atomic(struct test_object*) prev;
  
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
  
  struct descriptor* desc = descriptor_new(&type);
  struct managed_heap* heap = managed_heap_new(GC_NOP_GC, 1, params, 0);
  managed_heap_attach_context(heap);
  
  struct object* obj = managed_heap_alloc_object(desc);
  
  managed_heap_detach_context(heap);
  managed_heap_free(heap);
  descriptor_release(desc);
  return EXIT_SUCCESS;
}
