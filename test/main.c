#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <mimalloc.h>
#include <inttypes.h>
#include <stdint.h>

#include <flup/core/panic.h>
#include <flup/core/logger.h>

#include <flup/thread/thread.h>

#include "gc/driver.h"
#include "platform/platform.h"
#include "heap/heap.h"
#include "memory/alloc_tracker.h"
#include "object/descriptor.h"
#include "object/helper.h"
#include "util/cpu_pin.h"
#include "stat_printer.h"

#define WINDOW_SIZE 200'000
#define MESSAGE_COUNT 30'000'000
#define MESSAGE_SIZE (1 * 1024)

struct array_of_messages {
  long length;
  _Atomic(char*) messages[];
};

static int64_t worstTimeMicroSec = 0;
static int64_t totalTimeMicroSec = 0;
static int64_t sampleCount = 0;

static struct root_ref* newMessage(struct heap* heap, int n) {
  size_t size = MESSAGE_SIZE; //(size_t) ((float) rand() / (float) RAND_MAX * MESSAGE_SIZE);
  
  struct root_ref* message = heap_alloc(heap, size);
  memset(message->obj->data, n & 0xFF, size);
  return message;
}

static void pushMessage(struct heap* heap, struct root_ref* window, int id) {
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  
  struct root_ref* message = newMessage(heap, id);
  object_helper_write_ref(heap, window->obj, offsetof(struct array_of_messages, messages[id % WINDOW_SIZE]), message->obj);
  heap_root_unref(heap, message);
  
  clock_gettime(CLOCK_REALTIME, &end);
  
  int64_t diffSecondsInMicroSeconds = (end.tv_sec - start.tv_sec) * 1'000'000;
  int64_t diffNanosecsInMicroSeconds = (end.tv_nsec - start.tv_nsec) / 1'000;
  int64_t currentElapsedTimeInMicrosecs = diffSecondsInMicroSeconds + diffNanosecsInMicroSeconds;
  
  if (currentElapsedTimeInMicrosecs > worstTimeMicroSec)
    worstTimeMicroSec = currentElapsedTimeInMicrosecs;
  totalTimeMicroSec += currentElapsedTimeInMicrosecs;
  sampleCount++;
}

static struct descriptor desc_array_of_messages = {
  .hasFlexArrayField = true,
  .fieldCount = 0,
  .objectSize = sizeof(struct array_of_messages)
};

static void runTest(struct heap* heap, int iterations) {
  struct root_ref* messagesWindow = heap_alloc_with_descriptor(heap, &desc_array_of_messages, WINDOW_SIZE * sizeof(void*));
  struct array_of_messages* deref = (void*) messagesWindow->obj->data;
  deref->length = WINDOW_SIZE;
  
  // Warming up garbage collector first
  for (int i = 0; i < iterations; i++) {
    pushMessage(heap, messagesWindow, i);
    // if (i % 10 == 0) {
    //   clock_nanosleep(CLOCK_REALTIME, 0, &(struct timespec) {
    //     .tv_sec = 0,
    //     // 10 microsecond
    //     .tv_nsec = 100 * 1'000
    //   }, NULL);
    // }
  }
  heap_root_unref(heap, messagesWindow);
}

static void warmUp(struct heap* heap) {
  runTest(heap, 10'000'000);
  worstTimeMicroSec = 0;
  totalTimeMicroSec = 0;
  sampleCount = 0;
}

static void doTestGCExperiment(struct heap* heap) {
  runTest(heap, MESSAGE_COUNT);
  pr_info("Worst push time: %f milisecs", (float) worstTimeMicroSec / 1'000);
  pr_info("Average push time: %f milisecs", ((float) worstTimeMicroSec / (float) sampleCount) / 1'000);
}

int main() {
  if (!flup_attach_thread("Main-Thread"))
    flup_panic("Failed to attach thread\n");
  
  pr_info("Hello World!");
  pr_info("FluffyGC running on %s", platform_get_name());
  
  // Create 128 MiB heap
  size_t heapSize = 600 * 1024 * 1024;
  size_t reserveExtra = 64 * 1024 * 1024;
  
  if (mi_reserve_os_memory(heapSize + reserveExtra, true, true) != 0)
    flup_panic("Cannot reserve memory by mi_reserve_os_memory");
  
  struct heap* heap = heap_new(heapSize);
  if (!heap) {
    pr_error("Error creating heap");
    return EXIT_FAILURE;
  }
  
  pr_info("Pinning main thread to core 0");
  int ret = util_cpu_pin_try_pin(pthread_self(), 0);
  if (ret == -ENOSYS)
    pr_warn("CPU pinning is disabled");
  else if (ret == false)
    flup_panic("Error pinning main thread to core 0");
  
  pr_info("Pinning GC thread to core 1");
  ret = util_cpu_pin_try_pin(heap->gen->gcState->thread->thread, 1);
  if (ret == -ENOSYS)
    pr_warn("CPU pinning is disabled");
  else if (ret == false)
    flup_panic("Error pinning main thread to core 0");
  
  struct stat_printer* printer = NULL;
  if (!(printer = stat_printer_new(heap)))
    flup_panic("Failed to start stat printer!");
  
  size_t beforeTestBytesAllocated = atomic_load(&heap->gen->allocTracker->lifetimeBytesAllocated);
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);
  // Test based on same test on
  // https://github.com/WillSewell/gc-latency-experiment
  // in particular Java version is ported ^w^
  doTestGCExperiment(heap);
  // sleep(10);
  // doTestGCExperiment(heap);
  // sleep(10);
  // doTestGCExperiment(heap);
  // sleep(10);
  // doTestGCExperiment(heap);
  clock_gettime(CLOCK_REALTIME, &end);
  
  double startTime = (double) start.tv_sec + (double) start.tv_nsec / 1e9;
  double endTime = (double) end.tv_sec + (double) end.tv_nsec / 1e9;
  pr_info("Test duration was %lf sec", endTime - startTime);
  pr_info("And %lf MiB allocated during test time", ((double) (atomic_load(&heap->gen->allocTracker->lifetimeBytesAllocated) - beforeTestBytesAllocated)) / 1024.0f / 1024.0f);
  
  pr_info("Exiting... UwU");
  
  stat_printer_free(printer);
  
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

