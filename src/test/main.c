#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>

#include "FluffyHeap.h"
#include "logger/logger.h"
#include "attributes.h"
#include "bug.h"
#include "hook/hook.h"
#include "mods/dma.h"
#include "util/circular_buffer.h"
#include "util/util.h"
#include "macros.h"
#include "main.h"

#include "rcu/rcu.h"
#include "rcu/rcu_generic.h"
#include "rcu/rcu_generic_type.h"
#include "rcu/rcu_vec.h"
#include "vec.h"

struct test_type2 {
  int im;
  const char* a;
  long very;
  long long cute;
  _Atomic(struct test_type*) fox;
  char like;
  int me;
  long has;
  long fluffyTail;
};

fh_descriptor_field fields2[] = {
  FH_FIELD(struct test_type2, fox, "fox.fluffygc.Fluff", FH_REF_STRONG),
  FH_FIELD_END()
};

fh_descriptor_param descParam2 = {
  .fields = fields2,
  .size = sizeof(struct test_type2)
};

static int loader(const char* name, void* udata, fh_descriptor_param* param) {
  UNUSED(udata);
  if (strcmp(name, "fox.fluffygc.Test2") == 0) {
    *param = descParam2;
    return 0;
  }
  
  return -ESRCH;
}

[[maybe_unused]]
static void doTestNormal() {
  size_t sizes[] = {
    64 * 1024 * 1024
  };
  
  fh_param param = {
    .hint = FH_GC_HIGH_THROUGHPUT,
    .generationCount = 1,
    .generationSizes = sizes
  };
  
  int ret = fh_enable_mod(FH_MOD_DMA, 0);
  if (ret < 0) {
    errno = -ret;
    perror("fh_enable_mod");
    return;
  }
  
  ret = fh_enable_mod(FH_MOD_DEBUG, 0);
  if (ret < 0) {
    errno = -ret;
    perror("fh_enable_mod");
    return;
  }
  
  fluffyheap* heap = fh_new(&param);
  BUG_ON(!heap);
  
  fh_attach_thread(heap);
  fh_set_descriptor_loader(heap, loader);
  
  struct fluff {
    int im;
    const char* a;
    long very;
    long long cute;
    _Atomic(struct fluff*) fox;
    char like;
    int me;
    long has;
    long fluffyTail;
    _Atomic(struct test_type2*) fuwa;
    // Array not yet implemented into descriptor loader
    // _Atomic(struct fluff**) arrayOfFluffs;
    _Atomic(struct fluff**) any;
  };
  
  fh_descriptor_field fields[] = {
    FH_FIELD(struct fluff, fox, "fox.fluffygc.Fluff", FH_REF_STRONG),
    FH_FIELD(struct fluff, fuwa, "fox.fluffygc.Fluff", FH_REF_STRONG),
    FH_FIELD(struct fluff, any, "fox.fluffyheap.marker.Any", FH_REF_STRONG),
    FH_FIELD_END()
  };
  
  fh_descriptor_param descParam = {
    .fields = fields,
    .size = sizeof(struct fluff)
  };
  
  fh_descriptor* fluffDesc;
  fh_define_descriptor(&fluffDesc, "fox.fluffygc.Fluff", &descParam, false);
  //fh_define_descriptor("fox.fluffygc.Test2", &descParam2, false);
  
  // Object test
  {
    fh_object* obj = fh_alloc_object(fluffDesc);
  
    struct fluff data;
    fh_object_read_data(obj, &data, 0, sizeof(data));
    data.a = "C string test UwU";
    fh_object_write_data(obj, &data, 0, sizeof(data));
    fh_object_read_data(obj, &data, 0, sizeof(data));
    pr_alert("Got: %s", data.a);
    
    fh_object_write_ref(obj, offsetof(struct fluff, fox), obj);
    fh_object_write_ref(obj, offsetof(struct fluff, any), obj);
    
    fh_object* readVal = fh_object_read_ref(obj, offsetof(struct fluff, fox));
    pr_alert("Object is %ssame object", fh_object_is_alias(obj, readVal) ? "" : "not ");
    fh_del_ref(readVal);
    fh_del_ref(obj);
  }
  
  // Array test
  {
    fh_object* obj = fh_alloc_object(fluffDesc);
    
    fh_array* array = fh_alloc_array(fluffDesc, 5);
    fh_array_set_element(array, 0, obj);
    fh_object* readVal = fh_array_get_element(array, 0);
    pr_alert("Array[0] is %ssame what just written", fh_object_is_alias(obj, readVal) ? "" : "not ");
    
    // Triger warning (or abort) intentionally
    fh_array_calc_offset(array, 999);
    
    const fh_type_info* info = fh_object_get_type_info(FH_CAST_TO_OBJECT(array));
    pr_alert("Array is %zu has entries long", info->info.refArray->length);
    pr_alert("Array is %zu has entries according to API call", fh_array_get_length(array));
    fh_object_put_type_info(FH_CAST_TO_OBJECT(array), info);
    
    pr_alert("Not real array has %zu entries according to API call", fh_array_get_length(FH_CAST_TO_ARRAY(obj)));
    
    fh_del_ref((fh_object*) array);
    fh_del_ref((fh_object*) readVal);
    fh_del_ref((fh_object*) obj);
  }
  
  fh_release_descriptor(fluffDesc);
  
  // DMA Test
  {
    fh_object* obj = fh_alloc_object(fluffDesc);
    fh_dma_ptr* mapped = fh_object_map_dma(obj, 0, 0, 0, FH_MOD_DMA_ACCESS_RW);
    
    struct fluff* mappedPtr = mapped->ptr;
    mappedPtr->cute = 0x8739;
    
    fh_object_unmap_dma(obj, mapped);
    long long result;
    fh_object_read_data(obj, &result, offsetof(struct fluff, cute), sizeof(result));
    pr_alert("UwU Result 0x%llx", result);
    fh_del_ref(obj);
  }
  
  // Cleaning up
  fh_detach_thread(heap);
  fh_free(heap);
}

HOOK_FUNCTION(static, __FLUFFYHEAP_NULLABLE(fluffyheap*), hookTest, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams) {
  UNUSED(incomingParams);
  ci->action = HOOK_CONTINUE;
  pr_alert("Testing runtime adding");
  return;
}

ATTRIBUTE_USED()
static void doTestNormal2() {
  hook_register(fh_new, HOOK_HEAD, hookTest);
  
  doTestNormal();
  
  hook_unregister(fh_new, HOOK_HEAD, hookTest);
  // doTestNormal();
}

struct an_rcu_using_struct {
  struct rcu_head rcuHead;
  
  int uwu;
};

static struct rcu_struct rcuProtected;
static void* worker(void*) {
  uint64_t iteration = 0;
  pthread_t selfThread = pthread_self();
  while (1) {
    struct rcu_head* currentRcu = rcu_read_lock(&rcuProtected);
    struct an_rcu_using_struct* result = container_of(rcu_read(&rcuProtected), struct an_rcu_using_struct, rcuHead);
    fprintf(stderr, "Reader: %d -> %20" PRIu64 " (Thread: %20jx:)\r", result->uwu, iteration, (uintmax_t) selfThread);
    rcu_read_unlock(&rcuProtected,currentRcu);
    
    pthread_testcancel();
    iteration++;
  }
  return NULL;
}

static void freeRcuData(struct rcu_head* head) {
  struct an_rcu_using_struct* self = container_of(head, struct an_rcu_using_struct, rcuHead);
  free(self);
}

ATTRIBUTE_USED()
static void doTestRCU() {
  rcu_init(&rcuProtected);
  pthread_t new, new2;
  
  // Init
  {
    struct an_rcu_using_struct* currentData = malloc(sizeof(*currentData));
    *currentData = (struct an_rcu_using_struct) {};
    struct rcu_head* old = rcu_exchange_and_synchronize(&rcuProtected, &currentData->rcuHead);
    if (old)
      freeRcuData(old);
  }
  
  pthread_create(&new, NULL, worker, NULL);
  pthread_create(&new2, NULL, worker, NULL);
  
  for (int i = 0; i < 5; i++) {
    fprintf(stderr, "\033[2KWriter start writing %d into data\n", i);
    struct an_rcu_using_struct* newCopy = malloc(sizeof(*newCopy));
    
    // Create copy
    {
      struct rcu_head* currentRcu = rcu_read_lock(&rcuProtected);
      struct an_rcu_using_struct* current = container_of(rcu_read(&rcuProtected), struct an_rcu_using_struct, rcuHead);
      rcu_memcpy(&newCopy->rcuHead, &current->rcuHead, offsetof(struct an_rcu_using_struct, rcuHead), sizeof(*newCopy));
      rcu_read_unlock(&rcuProtected, currentRcu);
    }
    
    // Update
    newCopy->uwu = i;
    
    // Write, synchronize and clean old instance
    struct rcu_head* old = rcu_exchange_and_synchronize(&rcuProtected, &newCopy->rcuHead);
    freeRcuData(old);
    
    fprintf(stderr, "Writer written %d into data\n", i);
    sleep(1);
  }
  
  pthread_cancel(new);
  pthread_cancel(new2);
  pthread_join(new, NULL);
  pthread_join(new2, NULL);
  rcu_cleanup(&rcuProtected);
}

struct some_data_uwu {
  int UwU;
};

RCU_GENERIC_TYPE_DEFINE_TYPE(some_data_uwu_rcu, struct some_data_uwu);

static struct some_data_uwu_rcu rcuProtected2;
static void* worker2(void*) {
  uint64_t iteration = 0;
  pthread_t selfThread = pthread_self();
  while (1) {
    
    struct rcu_head* currentRcu = rcu_read_lock(&rcuProtected2.generic.rcu);
    struct some_data_uwu_rcu_readonly_container instance = rcu_generic_type_get(&rcuProtected2);
    fprintf(stderr, "Reader: %d -> %20" PRIu64 " (Thread: %20jx)\r", instance.data->UwU, iteration, (uintmax_t) selfThread);
    rcu_read_unlock(&rcuProtected2.generic.rcu, currentRcu);
    
    pthread_testcancel();
    iteration++;
  }
  return NULL;
}

ATTRIBUTE_USED()
static void doTestRCUGenericType() {
  rcu_generic_type_init(&rcuProtected2, NULL);
  
  // Init
  {
    struct some_data_uwu_rcu_writeable_container instance = rcu_generic_type_alloc_for(&rcuProtected2);
    instance.data->UwU = 0;
    rcu_generic_write(&rcuProtected2.generic, instance.container);
  }
  
  pthread_t new, new2;
  pthread_create(&new, NULL, worker2, NULL);
  pthread_create(&new2, NULL, worker2, NULL);
  
  for (int i = 0; i < 5; i++) {
    fprintf(stderr, "\033[2KWriter start writing %d into data\n", i);
    
    // Copy
    struct some_data_uwu_rcu_writeable_container copy = rcu_generic_type_copy(&rcuProtected2);
    
    // Update
    copy.data->UwU = i;
    
    // Write
    rcu_generic_write(&rcuProtected2.generic, copy.container);
    
    fprintf(stderr, "Writer written %d into data\n", i);
    sleep(1);
  }
  
  pthread_cancel(new);
  pthread_cancel(new2);
  pthread_join(new, NULL);
  pthread_join(new2, NULL);
  rcu_generic_type_cleanup(&rcuProtected2);
}

struct uwu {
  long UwU;
  char OwO[12];
};

RCU_VEC_DEFINE_TYPE(vec_hi_rcu, struct uwu);
static struct vec_hi_rcu rcuProtected3;
static void* worker3(void*) {
  uint64_t iteration = 0;
  pthread_t selfThread = pthread_self();
  UNUSED(iteration);
  UNUSED(selfThread);
  
  while (1) {
    struct rcu_head* currentRcu = rcu_read_lock(&rcuProtected3.generic.rcu);
    struct vec_hi_rcu_readonly_container readonly = rcu_generic_type_get(&rcuProtected3);
    
    int i = 0;
    struct uwu* current;
    vec_foreach_ptr(&readonly.data->array, current, i) {
      // fprintf(stderr, "[%ld] [%20ld] Got {%ld, \"%s\"} at %d\033[2K\r", selfThread, iteration, current->UwU, current->OwO, i);
      (void) current;
    }
    
    rcu_read_unlock(&rcuProtected3.generic.rcu, currentRcu);
    
    pthread_testcancel();
    iteration++;
  }
  return NULL;
}

ATTRIBUTE_USED()
static void doTestVecRCU() {
  rcu_generic_type_init(&rcuProtected3, &rcu_vec_t_ops);
  
  {
    struct vec_hi_rcu_writeable_container writeable = rcu_generic_type_alloc_for(&rcuProtected3);
    rcu_vec_init(&writeable);
    
    struct uwu data = (struct uwu) {
      .OwO = "Hello UwU!!"
    };
    vec_push(&writeable.data->array, data);
    
    rcu_generic_type_write(&rcuProtected3, &writeable);
  }
  
  pthread_t new, new2;
  pthread_create(&new, NULL, worker3, NULL);
  pthread_create(&new2, NULL, worker3, NULL);
  
  for (int i = 0; i < 5; i++) {
    fprintf(stderr, "\033[2KWriter start writing %d into data\n", i);
    
    // Copy
    struct vec_hi_rcu_writeable_container copy = rcu_generic_type_copy(&rcuProtected3);
    
    // Update
    struct uwu newUwU = {
      .UwU = i,
      .OwO = "Hello UwU!!"
    };
    vec_push(&copy.data->array, newUwU);
    
    // Write
    rcu_generic_type_write(&rcuProtected3, &copy);
    sleep(1);
  }
  
  pthread_cancel(new);
  pthread_cancel(new2);
  pthread_join(new, NULL);
  pthread_join(new2, NULL);
  rcu_generic_type_cleanup(&rcuProtected3);
}

static struct circular_buffer* buffer;

static void* worker4(void*) {
  float deadline = util_get_monotonic_time() + 1.0f;
  struct timespec sleepUntil;
  
  float writeSpeed = 1000.0f;
  float writePeriod = 1.0f / writeSpeed;
  int prev = 0;
  clock_gettime(CLOCK_MONOTONIC, &sleepUntil);
  for (int i = 0;; i++) {
    util_add_timespec(&sleepUntil, writePeriod);
    
    int result = rand();
    circular_buffer_write(buffer, 0, &result, sizeof(result), NULL);
    
    if (util_get_monotonic_time() >= deadline) {
      pr_alert("Writer: Speed %d writes/s", i - prev);
      prev = i;
      deadline = util_get_monotonic_time() + 1.0f;
    }
    
    util_sleep_until(&sleepUntil);
  }
  return NULL;
}

ATTRIBUTE_USED()
static void doTestCircularBuffer() {
  buffer = circular_buffer_new(16384);
  
  pthread_t worker;
  pthread_create(&worker, NULL, worker4, NULL);
  
  float readSpeed = 1.0f;
  float readPeriod = 1.0f / readSpeed;

  float deadline = util_get_monotonic_time() + 1.0f;
  struct timespec sleepUntil;
  int prev = 0;
  clock_gettime(CLOCK_MONOTONIC, &sleepUntil);
  for (int i = 0;; i++) {
    util_add_timespec(&sleepUntil, readPeriod);
    
    int result;
    circular_buffer_read(buffer, 0, &result, sizeof(result), NULL);
    
    if (util_get_monotonic_time() >= deadline) {
      pr_alert("Reader: Speed %d reads/s", i - prev);
      prev = i;
      deadline = util_get_monotonic_time() + 1.0f;
    }
    
    util_sleep_until(&sleepUntil);
  }
  
  pthread_join(worker, NULL);
  circular_buffer_free(buffer);
}

ATTRIBUTE_USED()
static void doTestSleep() {
  double prevTime;
  struct timespec sleepUntil;
  util_get_monotonic_timespec(&sleepUntil);
  prevTime = util_timespec_to_double(&sleepUntil);

  double sleepTime = 1.0f;
  while (1) {
    util_add_timespec(&sleepUntil, sleepTime);
    
    double currentTime = util_get_realtime_time();
    pr_alert("Time %lf diff in time: %lf", currentTime, currentTime - prevTime - sleepTime);
    
    prevTime = currentTime;
    util_sleep_until(&sleepUntil);
  }
}

ATTRIBUTE_USED()
static void doTestMonotonic() {
  double prev = util_get_monotonic_time();
  double current = util_get_monotonic_time();
  
  while (true) {
    pr_info("Delta %lf", current - prev);
    
    prev = current;
    current = util_get_monotonic_time();
  }
}

ATTRIBUTE_USED()
static void doTestBTree() {
  
}

// Runs forever
static void* loggerFunc(void*) {
  util_set_thread_name("Logger Thread");
  
  const char* logFilename = "./latest.log"; 
  FILE* logFile = NULL; //fopen(logFilename, "a");
  if (!logFile)
    pr_alert("Error opening '%s' log file, logs will not be logged", logFilename);
  pr_info("Logging into %s", logFilename);
  
  bool isLogBuffered = false;
  if (logFile && setvbuf(logFile, NULL, _IONBF, 0) < 0) {
    pr_error("Error setting log buffering, relying on explicit fflush");
    isLogBuffered = true;
  }
  
  while (1) {
    struct logger_entry entry;
    logger_get_entry(&entry);
    fputs(entry.message, stderr);
    fputs("\n", stderr);
    if (logFile) {
      fputs(entry.message, logFile);
      fputs("\n", logFile);
    }
    
    if (isLogBuffered)
      fflush(logFile);
  }
  
  fclose(logFile);
  return NULL;
}

int main2(int argc, const char** argv) {
  UNUSED(argc);
  UNUSED(argv);

  util_set_thread_name("Main Thread");
  logger_init();
  
  pthread_t loggerThread;
  int ret = 0;
  if ((ret = -pthread_create(&loggerThread, NULL, loggerFunc, NULL)) < 0)
    hard_panic("Logger thread can't start: %d", ret);
  pr_alert("Starting UwU");
  
  ret = hook_init();
  BUG_ON(ret < 0 && ret != -ENOSYS);
  
  // doTestBTree();
  // doTestMonotonic();
  // doTestSleep();
  // doTestCircularBuffer();
  doTestNormal2();
  // doTestRCU();
  // doTestRCUGenericType();
  // doTestVecRCU();
  return EXIT_SUCCESS;
}
