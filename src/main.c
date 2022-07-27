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
  char arr[32 KiB];
  struct somedata* data;
};

static void* abuser(void* _heap) {
  fluffygc_state* heap = _heap;
  fluffygc_v1_attach_thread(heap);
 
  fluffygc_field fields[] = {
    {
      .name = "data",
      .isArray = false,
      .type = FLUFFYGC_FIELD_STRONG,
      .offset = offsetof(struct somedata, data)
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
  fluffygc_v1_push_frame(heap, 16);

  ////////
  fluffygc_object* obj = fluffygc_v1_new_object(heap, desc);
  fluffygc_object* obj1 = fluffygc_v1_new_object(heap, desc);

  fluffygc_v1_set_object_field(heap, obj, offsetof(struct somedata, data), obj1);
  
  fluffygc_v1_delete_local_ref(heap, obj1);
  fluffygc_v1_delete_local_ref(heap, obj);
  ////////
  
  fluffygc_v1_pop_frame(heap, NULL);
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
        0.45f
      );

  int abuserCount = 6;
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



