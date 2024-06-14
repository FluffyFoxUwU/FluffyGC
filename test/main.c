#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <flup/data_structs/list_head.h>
#include <flup/core/logger.h>
#include <flup/thread/thread.h>

#include "heap/heap.h"

int main() {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  pr_info("Hello World!");
  
  // Create 64 MiB heap
  struct heap* heap = heap_new(64 * 1024 * 1024);
  if (!heap) {
    pr_error("Error creating heap");
    return EXIT_FAILURE;
  }
  
  // Intentionally leak 5 MiB for testing
  heap_alloc(heap, 5 * 1024 * 1024);
  // Try to allocate and release 256 MiB worth of items
  size_t bytesToAlloc = 256 * 1024 * 1024;
  size_t perItemSize = 64;
  for (size_t i = 0; i < bytesToAlloc / perItemSize; i++) {
    size_t sz = (size_t) (((float) rand() / (float) RAND_MAX) * (float) 1024);
    struct root_ref* ref = heap_alloc(heap, sz);
    heap_root_unref(heap, ref);
  }
  
  heap_free(heap);
  flup_thread_free(flup_detach_thread());
  // mimalloc_play();
}

// struct timespec start, end;
  // size_t numberOfItem = 10'000'000;
  
  // clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
  // flup_list_head head = FLUP_LIST_HEAD_INIT(head);
  
  // for (size_t i = 0; i < numberOfItem; i++) {
  //   flup_list_head* node = malloc(64);
  //   flup_list_add_tail(&head, node);
  // }
  // clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
  
  // double startTime, endTime;
  // startTime = (double) start.tv_sec + ((double) start.tv_nsec) / 1'000'000'000;
  // endTime = (double) end.tv_sec + ((double) end.tv_nsec) / 1'000'000'000;
  // pr_info("Created %zu nodes in %lf seconds", numberOfItem, endTime - startTime);
  
  // clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
  // flup_list_head* current;
  // flup_list_head* next;
  // flup_list_for_each_safe(&head, current, next) {
  //   if (current->next != NULL) {
  //     flup_list_del(current);
  //     free(current);
  //   }
  // }
  // clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
  
  // startTime = (double) start.tv_sec + ((double) start.tv_nsec) / 1'000'000'000;
  // endTime = (double) end.tv_sec + ((double) end.tv_nsec) / 1'000'000'000;
  // pr_info("Freeing %zu nodes took %lf seconds", numberOfItem, endTime - startTime);

