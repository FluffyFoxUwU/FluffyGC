#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bug.h"
#include "heap.h"
#include "soc.h"
#include "context.h"

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
  
  struct context* thread = context_new();
  context_current = thread;
  
  struct heap* heap = heap_new(32 * 1024 * 1024 /* 256 * 1024 * 1024 */);
  // Set parameters
  heap_param_set_local_heap_size(heap, 4 * 1024 * 1024);
  heap_init(heap);
  
  heap_on_thread_create(heap, thread);
  
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
      struct heap_block* block = heap_alloc(sizeof(struct obj));
      struct heap_block* block2 = heap_alloc(sizeof(struct obj) + 1);
      escape(block);
      escape(block2);
      heap_dealloc(block);
      heap_dealloc(block2);
    }
  }
  double timeMS = (double) (clock() - clk) / (double) CLOCKS_PER_SEC;
  printf("Time: %.2lf ms\n", timeMS);
  
  heap_merge_free_blocks(heap);
  heap_free(heap);
  context_free(thread);
  return EXIT_SUCCESS;
}


int main2(int argc, char** argv) {
  printf("Hello World!\n");
  
  struct context* thread = context_new();
  context_current = thread;
  
  struct heap* heap = heap_new(32 * 1024 * 1024 /* 256 * 1024 * 1024 */);
  // Set parameters
  heap_param_set_local_heap_size(heap, 4 * 1024 * 1024);
  heap_init(heap);
  
  heap_on_thread_create(heap, thread);
  
  
  
  heap_merge_free_blocks(heap);
  heap_free(heap);
  context_free(thread);
  return EXIT_SUCCESS;
}
