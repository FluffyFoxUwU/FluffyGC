#include <errno.h>
#include <pthread.h>
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
#include "test/common.h"

static char fwuffyAndLargeBufferUwU[16 * 1024 * 1024];

fgc_heap* fluffygc_test_heap_for_lua = NULL;

static struct heap* testHeap = NULL;
static atomic_bool shutdownRequested = false;
static FILE* statCSVFile;
static flup_thread* statWriter;
static void cleanup() {
  pr_info("Exiting... UwU");
  
  atomic_store(&shutdownRequested, true);
  flup_thread_wait(statWriter);
  flup_thread_free(statWriter);
  
  fclose(statCSVFile);
  heap_free(testHeap);
  flup_thread_free(flup_detach_thread());
}

int main(int argc, char** argv) {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  pr_info("Hello World!");
  
  // Create 128 MiB heap
  testHeap = heap_new(128 * 1024 * 1024);
  if (!testHeap) {
    pr_error("Error creating heap");
    return EXIT_FAILURE;
  }
  fluffygc_test_heap_for_lua = (void*) testHeap;
  
  statCSVFile = fopen("./benches/stat.csv", "a");
  if (!statCSVFile)
    flup_panic("Cannot open ./benches/stat.csv file for appending!");
  if (setvbuf(statCSVFile, fwuffyAndLargeBufferUwU, _IOFBF, sizeof(fwuffyAndLargeBufferUwU)) != 0)
    flup_panic("Error on setvbuf for ./benches/stat.csv");
  fprintf(statCSVFile, "Timestamp,Usage,Async Threshold,Metadata Usage,Non-metadata Usage,Max size,Mutator utilization,GC utilization\n");
  
  static clockid_t mutatorCPUClock;
  static clockid_t gcCPUClock;
  
  struct timespec testStartTimeSpec;
  clock_gettime(CLOCK_REALTIME, &testStartTimeSpec);
  static double testStartTime;
  testStartTime = (double) testStartTimeSpec.tv_sec + (double) testStartTimeSpec.tv_nsec / 1e9;
  
  pthread_getcpuclockid(pthread_self(), &mutatorCPUClock);
  pthread_getcpuclockid(testHeap->gen->gcState->thread->thread, &gcCPUClock);
  
  statWriter = flup_thread_new_with_block(^(void) {
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    
    struct timespec prevMutatorCPUTimeSpec;
    struct timespec prevGCCPUTimeSpec;
    clock_gettime(mutatorCPUClock, &prevMutatorCPUTimeSpec);
    clock_gettime(gcCPUClock, &prevGCCPUTimeSpec);
    
    double prevMutatorCPUTime;
    double prevGCCPUTime;
    prevGCCPUTime = (double) prevGCCPUTimeSpec.tv_sec + (double) prevGCCPUTimeSpec.tv_nsec / 1e9;
    prevMutatorCPUTime = (double) prevMutatorCPUTimeSpec.tv_sec + (double) prevMutatorCPUTimeSpec.tv_nsec / 1e9;
    
    while (!atomic_load(&shutdownRequested)) {
      // Collect in 10 ms interval
      deadline.tv_nsec += 10 * 1'000'000;
      if (deadline.tv_nsec > 1'000'000'000) {
        deadline.tv_nsec -= 1'000'000'000;
        deadline.tv_sec++;
      }
      
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      double time = (double) now.tv_sec + (double) now.tv_nsec / 1e9f;
      
      struct timespec mutatorCPUTimeSpec;
      struct timespec gcCPUTimeSpec;
      clock_gettime(mutatorCPUClock, &mutatorCPUTimeSpec);
      clock_gettime(gcCPUClock, &gcCPUTimeSpec);
      
      double mutatorCPUTime = (double)  mutatorCPUTimeSpec.tv_sec + (double) mutatorCPUTimeSpec.tv_nsec / 1e9;
      double gcCPUTime = (double)  gcCPUTimeSpec.tv_sec + (double) gcCPUTimeSpec.tv_nsec / 1e9;
      
      double mutatorCPUUtilization = (mutatorCPUTime - prevMutatorCPUTime) / 0.01f;
      double gcCPUUtilization =  (gcCPUTime - prevGCCPUTime) / 0.01f;
      if (mutatorCPUUtilization > 1.0f)
        mutatorCPUUtilization = 1.0f;
      if (gcCPUUtilization > 1.0f)
        gcCPUUtilization = 1.0f;
      
      size_t usage = atomic_load(&testHeap->gen->arena->currentUsage);
      size_t metadataUsage = atomic_load(&testHeap->gen->arena->metadataUsage);
      size_t nonMetadataUsage = atomic_load(&testHeap->gen->arena->nonMetadataUsage);
      
      size_t maxSize = testHeap->gen->arena->maxSize;
      size_t asyncCycleTriggerThreshold = (size_t) ((float) maxSize * testHeap->gen->gcState->asyncTriggerThreshold);
      fprintf(statCSVFile, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n", time - testStartTime, (double) usage / 1024 / 1024, (double) asyncCycleTriggerThreshold / 1024 / 1024, (double) metadataUsage / 1024 / 1024, (double) nonMetadataUsage / 1024 / 1024, (double) maxSize / 1024 / 1024, mutatorCPUUtilization * 100.0f, gcCPUUtilization * 100.0f);
      
      prevMutatorCPUTime = mutatorCPUTime;
      prevGCCPUTime = gcCPUTime;
      
      while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &deadline, NULL) == EINTR)
        ;
    }
  });
  
  atexit(cleanup);
  return runLua(argc, argv);
}
