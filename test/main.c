#include <errno.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <flup/core/panic.h>
#include <flup/core/logger.h>

#include <flup/thread/thread.h>

#include "heap/heap.h"
#include "memory/arena.h"
#include "lua.h"

static char fwuffyAndLargeBufferUwU[16 * 1024 * 1024];

struct heap* heap;
static atomic_bool shutdownRequested = false;
FILE* statCSVFile;
flup_thread* statWriter;
static void cleanup() {
  pr_info("Exiting... UwU");
  
  atomic_store(&shutdownRequested, true);
  flup_thread_wait(statWriter);
  flup_thread_free(statWriter);
  
  fclose(statCSVFile);
  heap_free(heap);
  flup_thread_free(flup_detach_thread());
}

int main(int argc, char** argv) {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  pr_info("Hello World!");
  
  // Create 128 MiB heap
  heap = heap_new(128 * 1024 * 1024);
  if (!heap) {
    pr_error("Error creating heap");
    return EXIT_FAILURE;
  }
  
  statCSVFile = fopen("./benches/stat.csv", "a");
  if (!statCSVFile)
    flup_panic("Cannot open ./benches/stat.csv file for appending!");
  if (setvbuf(statCSVFile, fwuffyAndLargeBufferUwU, _IOFBF, sizeof(fwuffyAndLargeBufferUwU)) != 0)
    flup_panic("Error on setvbuf for ./benches/stat.csv");
  fprintf(statCSVFile, "timestamp_sec,timestamp_nanosec,heap_usage,gc_trigger_threshold,heap_total_size\n");
  
  statWriter = flup_thread_new_with_block(^(void) {
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    while (!atomic_load(&shutdownRequested)) {
      // Collect in 10 ms interval
      deadline.tv_nsec += 10 * 1'000'000;
      if (deadline.tv_nsec > 1'000'000'000) {
        deadline.tv_nsec -= 1'000'000'000;
        deadline.tv_sec++;
      }
      
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      
      size_t usage = atomic_load(&heap->gen->arena->currentUsage);
      size_t maxSize = heap->gen->arena->maxSize;
      size_t asyncCycleTriggerThreshold = (size_t) ((float) maxSize * heap->gen->gcState->asyncTriggerThreshold);
      fprintf(statCSVFile, "%llu,%llu,%zu,%zu,%zu\n", (unsigned long long) now.tv_sec, (unsigned long long) now.tv_nsec, usage, asyncCycleTriggerThreshold, maxSize);
      
      while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &deadline, NULL) == EINTR)
        ;
    }
  });
  
  atexit(cleanup);
  return runLua(argc, argv);
}
