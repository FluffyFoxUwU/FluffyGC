#include <linux/prctl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>

#include "config.h"
#include "region.h"
#include "descriptor.h"
#include "heap.h"
#include "thread.h"
#include "root.h"
#include "config.h"
#include "profiler.h"

#define KiB * (1024)
#define MiB * (1024 KiB)
#define GiB * (1024 MiB)

/*
Minor collection uses mark-sweep    (Only in young)
Major collection uses mark-sweep    (Only in old)
Full  collection uses mark-compact  (Only in old)

Minor GC trigger if any is true
1. Allocation failure

Major GC trigger if any is true
1. There is allocation failure 
   while promoting object to old 
   generation
2. Allocation failure in old
   generation

Full GC trigger if any is true
1. There another promotion failure
2. Old allocation still fails
3. Explicit GC

Declare OOM if
1. Still unable to promote objects
2. Direct old allocation still fails

mark-compact essentially mark-sweep followed by
compaction

Notes:
 * Young allocation uses bump pointer only
 * Old allocation uses bump pointer then
   fallback to find best fit allocator
*/

struct somedata {
  int someInteger;
  char arr[32 KiB];
  struct somedata* data;
};

static void* abuser(void* _heap) {
  prctl(PR_SET_NAME, "Abuser");
  
  struct descriptor_typeid id = {
    .name = "net.fluffyfox.bettergc.Test",
    .ownerID = 0,
    .typeID = 0
  };
  struct descriptor_field fields[] = {
    {
      .name = "data",
      .offset = offsetof(struct somedata, data),
      .type = DESCRIPTOR_FIELD_TYPE_STRONG
    }
  };
 
  struct heap* heap = _heap;
  struct descriptor* desc = heap_descriptor_new(heap, id, sizeof(struct somedata), sizeof(fields) / sizeof(*fields), fields);
  
  heap_attach_thread(heap);
  struct thread* currentThread = heap_get_thread_data(heap)->thread;
  
  //////////////////////
  struct root_reference* grandfather = heap_obj_new(heap, desc); 
  struct root_reference* grandson = heap_obj_new(heap, desc); 
  heap_obj_write_ptr(heap, grandfather, offsetof(struct somedata, data), grandson);
  heap_obj_write_ptr(heap, grandson, offsetof(struct somedata, data), grandson);

  {
    int integer = 32;
    heap_obj_write_data(heap, grandson, offsetof(struct somedata, someInteger), &integer, sizeof(integer));
  }

  heap_enter_unsafe_gc(heap);
  printf("Main: 1 %p\n", grandson->data->data);
  printf("Main: 2 %p\n", heap_obj_read_ptr(heap, grandfather, offsetof(struct somedata, data))->data->data);
  heap_exit_unsafe_gc(heap);
    
  struct root_reference* arr = heap_array_new(heap, 8);
  heap_array_write(heap, arr, 0, arr);
  heap_array_write(heap, arr, 1, grandfather);
  heap_array_write(heap, arr, 2, grandson);

  heap_enter_unsafe_gc(heap);
  thread_local_remove(currentThread, grandfather);
  thread_local_remove(currentThread, grandson);
  heap_exit_unsafe_gc(heap);
  
  for (int i = 0; i < 50000; i++) {
    grandson = heap_array_read(heap, arr, 2);
  
    struct root_reference* obj = heap_obj_new(heap, desc); 
    heap_obj_write_ptr(heap, grandson, offsetof(struct somedata, data), obj);

    struct root_reference* opaque = heap_obj_opaque_new(heap, 1 MiB);
    heap_enter_unsafe_gc(heap);
    thread_local_remove(currentThread, opaque);

    thread_local_remove(currentThread, obj);
    thread_local_remove(currentThread, grandson);
    heap_exit_unsafe_gc(heap);
  }

  grandfather = heap_array_read(heap, arr, 1);
  grandson = heap_array_read(heap, arr, 2);
 
  {
    int integer = -1;
    heap_obj_read_data(heap, grandson, offsetof(struct somedata, someInteger), &integer, sizeof(integer));
    printf("Main: grandson->someInteger = %d\n", integer);
  }
  //////////////////////
  
  heap_descriptor_release(heap, desc);
  heap_detach_thread(heap);
  
  return NULL;
}

int main2() {
  prctl(PR_SET_NAME, "Main");

  puts("Hello World!");
  printf("FluffyGC Ver %d.%d.%d\n", CONFIG_VERSION_MAJOR, CONFIG_VERSION_MINOR, CONFIG_VERSION_PATCH);


  // Young is 1/3 of total
  // Old   is 2/3 of total
  struct heap* heap = heap_new(8 MiB, 24 MiB, 32 KiB, 100, 0.45f, 65536);
  assert(heap);
  
  int abuserCount = 6;
  pthread_t* abusers = calloc(abuserCount, sizeof(*abusers));

  for (int i = 0; i < abuserCount; i++)
    if (pthread_create(&abusers[i], NULL, abuser, heap) != 0)
      abort();
  
  for (int i = 0; i < abuserCount; i++)
    pthread_join(abusers[i], NULL);

  free(abusers);
  heap_free(heap);
  return 0;
}

static void* testWorker(void* res) {
  *((int*) res) = main2();
  return NULL;
}

int main() {
  int res = 0;
  pthread_t tmp;
  pthread_create(&tmp, NULL, testWorker, &res);
  pthread_join(tmp, NULL);

  puts("Exiting :3");
  return res;
}

#if IS_ENABLED(CONFIG_ASAN)
const char* __asan_default_options() {
  return "fast_unwind_on_malloc=0:"
         "detect_invalid_pointer_pairs=10:"
         "strict_string_checks=1:"
         "strict_init_order=1:"
         "check_initialization_order=1:"
         "print_stats=1:"
         "detect_stack_use_after_return=1:"
         "atexit=1";
}
#endif

#if IS_ENABLED(CONFIG_UBSAN)
const char* __ubsan_default_options() {
  return "print_stacktrace=1:"
         "suppressions=suppressions/UBSan.supp";
}
#endif

#if IS_ENABLED(CONFIG_TSAN)
const char* __tsan_default_options() {
  return "second_deadlock_stack=1";
}
#endif

#if IS_ENABLED(CONFIG_MSAN)
const char* __msan_default_options() {
  return "";
}
#endif



