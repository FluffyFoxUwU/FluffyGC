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
#include "object/descriptor.h"
#include "object/helper.h"

static char fwuffyAndLargeBufferUwU[16 * 1024 * 1024];

int main() {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  pr_info("Hello World!");
  
  // Create 128 MiB heap
  struct heap* heap = heap_new(128 * 1024 * 1024);
  if (!heap) {
    pr_error("Error creating heap");
    return EXIT_FAILURE;
  }
  
  FILE* statCSVFile = fopen("./benches/stat.csv", "a");
  if (!statCSVFile)
    flup_panic("Cannot open ./benches/stat.csv file for appending!");
  if (setvbuf(statCSVFile, fwuffyAndLargeBufferUwU, _IOFBF, sizeof(fwuffyAndLargeBufferUwU)) != 0)
    flup_panic("Error on setvbuf for ./benches/stat.csv");
  fprintf(statCSVFile, "Timestamp,Usage,Async Threshold,Metadata Usage,Non-metadata Usage,Max size,Mutator utilization,GC utilization\n");
  
  static atomic_bool shutdownRequested = false;
  static clockid_t mutatorCPUClock;
  static clockid_t gcCPUClock;
  
  struct timespec testStartTimeSpec;
  clock_gettime(CLOCK_REALTIME, &testStartTimeSpec);
  static double testStartTime;
  testStartTime = (double) testStartTimeSpec.tv_sec + (double) testStartTimeSpec.tv_nsec / 1e9;
  
  pthread_getcpuclockid(pthread_self(), &mutatorCPUClock);
  pthread_getcpuclockid(heap->gen->gcState->thread->thread, &gcCPUClock);
  
  flup_thread* statWriter = flup_thread_new_with_block(^(void) {
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
      
      size_t usage = atomic_load(&heap->gen->arena->currentUsage);
      size_t metadataUsage = atomic_load(&heap->gen->arena->metadataUsage);
      size_t nonMetadataUsage = atomic_load(&heap->gen->arena->nonMetadataUsage);
      
      size_t maxSize = heap->gen->arena->maxSize;
      size_t asyncCycleTriggerThreshold = (size_t) ((float) maxSize * heap->gen->gcState->asyncTriggerThreshold);
      fprintf(statCSVFile, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n", time - testStartTime, (double) usage / 1024 / 1024, (double) asyncCycleTriggerThreshold / 1024 / 1024, (double) metadataUsage / 1024 / 1024, (double) nonMetadataUsage / 1024 / 1024, (double) maxSize / 1024 / 1024, mutatorCPUUtilization * 100.0f, gcCPUUtilization * 100.0f);
      
      prevMutatorCPUTime = mutatorCPUTime;
      prevGCCPUTime = gcCPUTime;
      
      while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &deadline, NULL) == EINTR)
        ;
    }
  });
  
  struct array {
    size_t length;
    size_t capacity;
    _Atomic(struct arena_block*) array[];
  };
  
  struct number {
    int content;
  };
  
  static struct descriptor arrayDesc = {
    .fieldCount = 0,
    .hasFlexArrayField = true,
    .objectSize = sizeof(struct array)
  };
  
  static void (^reserveArray)(struct heap* heap, struct root_ref** arrayRef, size_t length) = ^(struct heap* heap, struct root_ref** arrayRef, size_t length) {
    struct array* array = (*arrayRef)->obj->data;
    
    // Expand the array
    if (length > array->capacity) {
      size_t capacityNeeded = 1;
      while (capacityNeeded < length)
        capacityNeeded *=2;
      
      struct root_ref* copyOfArrayRef = heap_alloc_with_descriptor(heap, &arrayDesc, capacityNeeded * sizeof(void*));
      struct array* copyOfArray = copyOfArrayRef->obj->data;
      copyOfArray->length = array->length;
      copyOfArray->capacity = capacityNeeded;
      
      for (size_t i = 0; i < array->length; i++) {
        struct root_ref* currentRef = object_helper_read_ref(heap, (*arrayRef)->obj, offsetof(struct array, array[i]));
        object_helper_write_ref(heap, copyOfArrayRef->obj, offsetof(struct array, array[i]), currentRef);
        heap_root_unref(heap, currentRef);
      }
      
      heap_root_unref(heap, *arrayRef);
      *arrayRef = copyOfArrayRef;
    }
  };
  
  static void (^appendArray)(struct heap* heap, struct root_ref** arrayRef, struct root_ref* data) = ^(struct heap* heap, struct root_ref** arrayRef, struct root_ref* data) {
    struct array* array = (*arrayRef)->obj->data;
    
    if (array->length + 1 > array->capacity) {
      flup_panic("Cant occur");
      reserveArray(heap, arrayRef, array->length + 1);
    }
    
    array = (*arrayRef)->obj->data;
    // Actually write to array
    object_helper_write_ref(heap, (*arrayRef)->obj, offsetof(struct array, array[array->length]), data);
    array->length++;
  };
  
  static struct root_ref* (^newArray)(struct heap* heap) = ^struct root_ref* (struct heap* heap) {
    struct root_ref* arrayRef = heap_alloc_with_descriptor(heap, &arrayDesc, 0);
    struct array* array = arrayRef->obj->data;
    array->length = 0;
    array->capacity = 0;
    return arrayRef;
  };
  
  // Roughly like in Lua (ported from https://github.com/zenkj/luagctest/blob/master/src/main.lua)
  // and removed/substitute some which not available
  /*
  function test()
    local t = {}

    for i=1,1000 do
      t[i] = {}
      for j=1,100 do
        local var = {}
        var[1] = {812}
        var[2] = {var}
        
        local var2 = {}
        var2[1] = {1, 2, 3}
        var2[2] = {452}
        t[i][j] = var
      end
    end
  end
  */
  
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  for (size_t n = 0; n < 20; n++) {
    // local t = {}
    struct root_ref* t = newArray(heap);
    reserveArray(heap, &t, 1000);
    for (size_t i = 0; i < 1000; i++) {
      // t[i] = {}
      struct root_ref* temp0 = newArray(heap);
      reserveArray(heap, &temp0, 100);
      appendArray(heap, &t, temp0);
      heap_root_unref(heap, temp0);
      
      for (size_t j = 0; j < 100; j++) {
        // local var = {}
        struct root_ref* var = newArray(heap);
        reserveArray(heap, &var, 2);
        
        // var[1] = {812}
        {
          struct root_ref* temp1 = newArray(heap);
          reserveArray(heap, &temp1, 1);
          
          // {812}
          {
            struct root_ref* temp2 = heap_alloc(heap, sizeof(struct number));
            struct number* num = temp2->obj->data;
            num->content = 812;
            appendArray(heap, &temp1, temp2);
            heap_root_unref(heap, temp2);
          }
          
          appendArray(heap, &var, temp1);
          heap_root_unref(heap, temp1);
        }
        
        // var[2] = {var}
        appendArray(heap, &var, var);
        
        // local var2 = {}
        struct root_ref* var2 = newArray(heap);
        reserveArray(heap, &var2, 2);
        
        // var2[1] = {1, 2, 3}
        {
          struct root_ref* temp1 = newArray(heap);
          reserveArray(heap, &temp1, 3);
          
          for (int numCounter = 1; numCounter <= 3; numCounter++) {
            struct root_ref* temp2 = heap_alloc(heap, sizeof(struct number));
            struct number* num = temp2->obj->data;
            num->content = numCounter;
            appendArray(heap, &temp1, temp2);
            heap_root_unref(heap, temp2);
          }
          
          appendArray(heap, &var2, temp1);
          heap_root_unref(heap, temp1);
        }
        
        // var2[2] = {452}
        {
          struct root_ref* temp1 = newArray(heap);
          reserveArray(heap, &temp1, 1);
          
          struct root_ref* temp2 = heap_alloc(heap, sizeof(struct number));
          struct number* num = temp2->obj->data;
          num->content = 452;
          appendArray(heap, &temp1, temp2);
          
          heap_root_unref(heap, temp2);
          
          appendArray(heap, &var2, temp1);
          heap_root_unref(heap, temp1);
        }
        
        heap_root_unref(heap, var2);
        
        // t[i][j] = var
        {
          struct root_ref* temp5 = object_helper_read_ref(heap, t->obj, offsetof(struct array, array[i]));
          appendArray(heap, &temp5, var);
          heap_root_unref(heap, temp5);
        }
        
        heap_root_unref(heap, var);
      }
    }
    heap_root_unref(heap, t);
  }
  clock_gettime(CLOCK_REALTIME, &end);
  
  double startTime = (double) start.tv_sec + (double) start.tv_nsec / 1e9;
  double endTime = (double) end.tv_sec + (double) end.tv_nsec / 1e9;
  pr_info("Test duration was %lf sec", endTime - startTime);
  pr_info("And %lf MiB allocated during lifetime", ((double) atomic_load(&heap->gen->arena->lifetimeBytesAllocated)) / 1024.0f / 1024.0f);
  
  pr_info("Exiting... UwU");
  
  atomic_store(&shutdownRequested, true);
  flup_thread_wait(statWriter);
  flup_thread_free(statWriter);
  
  fclose(statCSVFile);
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

