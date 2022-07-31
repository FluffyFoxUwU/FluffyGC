#if 0
# include "main2.c"
#else

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

#include "config.h"
#include "fluffygc/v1.h"

#define KiB * (1024)
#define MiB * (1024 KiB)
#define GiB * (1024 MiB)

struct somedata {
  int someInteger;
  char arr[64 KiB];

  struct somedata* data;
  struct somedata** array;
  struct somedata* weakData;
};

static void* abuser(void* _heap) {
  fluffygc_state* heap = _heap;
  fluffygc_v1_attach_thread(heap);
 
  fluffygc_field fields[] = {
    {
      .name = "array",
      .dataType = FLUFFYGC_TYPE_ARRAY,
      .type = FLUFFYGC_FIELD_STRONG,
      .offset = offsetof(struct somedata, array)
    },
    {
      .name = "data",
      .dataType = FLUFFYGC_TYPE_NORMAL,
      .type = FLUFFYGC_FIELD_STRONG,
      .offset = offsetof(struct somedata, data)
    },
    {
      .name = "weakData",
      .dataType = FLUFFYGC_TYPE_NORMAL,
      .type = FLUFFYGC_FIELD_WEAK,
      .offset = offsetof(struct somedata, weakData)
    }
  };

  fluffygc_descriptor_args descriptorArgs = {
    .name = "net.fluffyfox.fluffygc.Test",
    .ownerID = 0,
    .typeID = 0,
    .objectSize = sizeof(struct somedata),
    .fieldCount = sizeof(fields) / sizeof(*fields),
    .fields = fields
  };
  fluffygc_descriptor* desc = fluffygc_v1_descriptor_new(heap, &descriptorArgs);

  ////////
  fluffygc_object_array* obj1 = fluffygc_v1_new_object_array(heap, 9);
  
  fluffygc_object* obj = fluffygc_v1_new_object(heap, desc);
  fluffygc_object* global = fluffygc_v1_new_global_ref(heap, obj);
  fluffygc_v1_delete_local_ref(heap, obj);

  fluffygc_v1_set_array_field(heap, global, offsetof(struct somedata, array), obj1);
  
  fluffygc_v1_delete_local_ref(heap, obj1);
  fluffygc_v1_delete_global_ref(heap, global);
  ////////
 
  fluffygc_v1_descriptor_delete(heap, desc);

  fluffygc_v1_detach_thread(heap);
  return NULL;
}

static int main2() {
  // Young is 1/3 of total
  // Old   is 2/3 of total
  fluffygc_state* heap = fluffygc_v1_new(
        8 MiB,
        24 MiB,
        32 KiB,
        100,
        0.45f,
        65536
      );

  int abuserCount = 6;
  (void) abuserCount;
  abuser(heap);
  fluffygc_v1_free(heap);
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


#endif
