#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "reference.h"
#include "region.h"
#include "descriptor.h"
#include "heap.h"
#include "thread.h"
#include "root.h"

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
  char arr[8 KiB];
  struct somedata* data;
};

int i;
int main2() {
  puts("Hello World!");
  printf("FluffyGC Ver %d.%d.%d\n", FLUFFYGC_VERSION_MAJOR, FLUFFYGC_VERSION_MINOR, FLUFFYGC_VERSION_PATCH);

  struct descriptor_typeid id = {
    .name = "net.fluffyfox.bettergc.Test",
    .ownerID = 0,
    .typeID = 0
  };
  struct descriptor_field fields[] = {
    {
      .name = "data",
      .offset = offsetof(struct somedata, data)
    }
  };
  struct heap* heap = heap_new(8 MiB, 200 KiB, 32 KiB, 100, 0.45f);
  assert(heap);

  heap_attach_thread(heap);
  struct thread* currentThread = heap_get_thread(heap);

  struct descriptor* desc = heap_descriptor_new(heap, id, sizeof(struct somedata), sizeof(fields) / sizeof(*fields), fields);
  
  //////////////////////
  thread_push_frame(currentThread, 16);
  struct root_reference* grandfather = heap_obj_new(heap, desc); 
  struct root_reference* grandson = heap_obj_new(heap, desc); 
  heap_obj_write_ptr(heap, grandfather, offsetof(struct somedata, data), grandson);
  heap_obj_write_ptr(heap, grandson, offsetof(struct somedata, data), grandson);

  {
    int integer = 32;
    heap_obj_write_data(heap, grandson, offsetof(struct somedata, someInteger), &integer, sizeof(integer));
  }

  printf("Main: 1 %p\n", grandson->data->data);
  printf("Main: 2 %p\n", heap_obj_read_ptr(heap, grandfather, offsetof(struct somedata, data))->data->data);
  
  //   500 with poison
  // 1,500 with no poison

  // 15,000 to fill old gen
  // 45,000 to fill old gen 3 times
  for (i = 0; i < 45000; i++) {
    struct root_reference* obj = heap_obj_new(heap, desc); 
    heap_obj_write_ptr(heap, grandson, offsetof(struct somedata, data), obj);

    thread_local_remove(currentThread, obj);
  }

  printf("Main: 1 %p\n", grandson->data->data);
  printf("Main: 2 %p\n", heap_obj_read_ptr(heap, grandfather, offsetof(struct somedata, data))->data->data);
  printf("Main: 3 %p\n", heap_obj_read_ptr(heap, grandson, offsetof(struct somedata, data))->data->data);
  
  {
    int integer = -1;
    heap_obj_read_data(heap, grandson, offsetof(struct somedata, someInteger), &integer, sizeof(integer));
    printf("Main: grandson->someInteger = %d\n", integer);
  }
  
  thread_pop_frame(currentThread, NULL);
  //////////////////////
  
  heap_descriptor_release(heap, desc);
  
  heap_detach_thread(heap);
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

#if FLUFFYGC_ASAN_ENABLED
// Force always slow unwind
const char* __asan_default_options() {
  return "fast_unwind_on_malloc=0:"
         "detect_invalid_pointer_pairs=10:"
         "strict_string_checks=1:"
         "strict_init_order=1:"
         "print_stats=1:"
         "atexit=1";
}
#endif

#if FLUFFYGC_UBSAN_ENABLED
const char* __ubsan_default_options() {
  return "print_stacktrace=1";
}
#endif




