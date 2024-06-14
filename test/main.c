// #include <errno.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
// #include <time.h>
#include <unistd.h>

#include <flup/core/panic.h>
#include <flup/core/logger.h>

#include <flup/data_structs/list_head.h>
#include <flup/thread/thread.h>

#include "heap/heap.h"

// static char fwuffyAndLargeBufferUwU[16 * 1024 * 1024];

int main() {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  pr_info("Hello World!");
  
  // Create 64 MiB heap
  struct heap* heap = heap_new(128 * 1024 * 1024);
  if (!heap) {
    pr_error("Error creating heap");
    return EXIT_FAILURE;
  }
  
  // FILE* statCSVFile = fopen("stat2.csv", "a");
  // if (!statCSVFile)
  //   flup_panic("Cannot open ./stat2.csv file for appending!");
  // if (setvbuf(statCSVFile, fwuffyAndLargeBufferUwU, _IOFBF, sizeof(fwuffyAndLargeBufferUwU)) != 0)
  //   flup_panic("Error on setvbuf for stat2.csv");
  // fprintf(statCSVFile, "timestamp_sec,timestamp_nanosec,heap_usage,heap_total_size\n");
  
  // static atomic_bool shutdownRequested = false;
  // flup_thread* statWriter = flup_thread_new_with_block(^(void) {
    // struct timespec deadline;
    // clock_gettime(CLOCK_REALTIME, &deadline);
    // while (!atomic_load(&shutdownRequested)) {
    //   // Collect in 100 ms interval
    //   deadline.tv_nsec += 100 * 1'000'000;
    //   if (deadline.tv_nsec > 1'000'000'000) {
    //     deadline.tv_nsec -= 1'000'000'000;
    //     deadline.tv_sec++;
    //   }
      
    //   struct timespec now;
    //   clock_gettime(CLOCK_REALTIME, &now);
      
    //   size_t usage = atomic_load(&heap->gen->arena->currentUsage);
    //   size_t maxSize = heap->gen->arena->maxSize;
    //   fprintf(statCSVFile, "%llu,%llu,%zu,%zu\n", (unsigned long long) now.tv_sec, (unsigned long long) now.tv_nsec, usage, maxSize);
      
    //   while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &deadline, NULL) == EINTR)
    //     ;
    // }
  // });
  
  // Intentionally leak 5 MiB for testing
  heap_alloc(heap, 5 * 1024 * 1024);
  // Try to allocate and release 256 MiB worth of items
  size_t bytesToAlloc = 256UL * 1024 * 1024;
  size_t perItemSize = 64;
  for (size_t i = 0; i < bytesToAlloc / perItemSize; i++) {
    size_t sz = (size_t) (((float) rand() / (float) RAND_MAX) * (float) 1024);
    struct root_ref* ref = heap_alloc(heap, sz);
    heap_root_unref(heap, ref);
    
    // if (i % 1000 == 0)
    //   pr_info("Progress: %zu out of %zu items", i, bytesToAlloc / perItemSize);
  }
  
  pr_info("Exiting... UwU");
  
  // atomic_store(&shutdownRequested, true);
  // flup_thread_wait(statWriter);
  // flup_thread_free(statWriter);
  
  // fclose(statCSVFile);
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

