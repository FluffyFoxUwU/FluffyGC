#if 1
# include "main2.c"
#else

#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "config.h"
#include "util.h"
#include "FluffyGC/v1.h"
#include "userfaultfd.h"

#define KiB * (1024)
#define MiB * (1024 KiB)
#define GiB * (1024 MiB)

struct somedata {
  struct somedata* weakData;
  int someInteger;
  char arr[64 KiB];

  struct somedata* data;
  struct somedata** array;
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

  static void* testKey;
  static void* typeKey;

  fluffygc_descriptor_args descriptorArgs = {
    .name = "net.fluffyfox.fluffygc.Test",
    .ownerID = (uintptr_t) &testKey,
    .typeID = (uintptr_t) &typeKey,
    .objectSize = sizeof(struct somedata),
    .fieldCount = sizeof(fields) / sizeof(*fields),
    .fields = fields
  };
  fluffygc_descriptor* desc = fluffygc_v1_descriptor_new(heap, &descriptorArgs);
  
  ////////
  fluffygc_object_array* obj1 = fluffygc_v1_new_object_array(heap, 9);
  fluffygc_v1_trigger_full_gc(heap);
  
  fluffygc_object* obj = fluffygc_v1_new_object(heap, desc);
  fluffygc_write_data(heap, obj, struct somedata, someInteger, 9);
  fluffygc_v1_trigger_full_gc(heap);

  fluffygc_object* global = fluffygc_v1_new_global_ref(heap, obj);
  fluffygc_v1_trigger_full_gc(heap);
  assert(fluffygc_v1_is_same_object(heap, obj, global) == true);
  fluffygc_v1_delete_local_ref(heap, obj);

  fluffygc_v1_trigger_full_gc(heap);

  fluffygc_v1_set_array_field(heap, global, offsetof(struct somedata, array), obj1);
  
  fluffygc_object* obj2 = fluffygc_v1_new_object(heap, desc);
  fluffygc_weak_object* obj2Weak = fluffygc_v1_new_weak_global_ref(heap, obj2);
  fluffygc_v1_set_object_field(heap, global, offsetof(struct somedata, weakData), obj2);
  {
    fluffygc_object* tmp;
    assert((tmp = fluffygc_v1_get_object_field(heap, global, offsetof(struct somedata, weakData))) != NULL);
    fluffygc_v1_delete_local_ref(heap, tmp);
  }
  
  fluffygc_v1_trigger_full_gc(heap);
  {
    fluffygc_object* tmp;
    assert((tmp = fluffygc_v1_get_object_field(heap, global, offsetof(struct somedata, weakData))) != NULL);
    fluffygc_v1_delete_local_ref(heap, tmp);
  }
  fluffygc_v1_delete_local_ref(heap, obj2);

  fluffygc_v1_trigger_full_gc(heap);
  
  assert(fluffygc_v1_is_same_object(heap, obj1, fluffygc_v1_get_array_field(heap, global, offsetof(struct somedata, array))) == true);
  
  volatile fluffygc_object* tmp;
  assert((tmp = fluffygc_v1_get_object_field(heap, global, offsetof(struct somedata, weakData))) == NULL);
  fluffygc_v1_trigger_full_gc(heap);
  
  {
    int tmp = -1;
    fluffygc_read_data(heap, &tmp, global, struct somedata, someInteger);
    printf("%d\n", tmp);
    assert(tmp == 9);
  }
  
  fluffygc_v1_delete_local_ref(heap, obj1);
  fluffygc_v1_delete_global_ref(heap, global);
  fluffygc_v1_delete_weak_global_ref(heap, obj2Weak);
  fluffygc_v1_trigger_full_gc(heap);
  ////////
 
  fluffygc_v1_descriptor_delete(heap, desc);

  fluffygc_v1_detach_thread(heap);
  return NULL;
}

int main2() {
  util_set_thread_name("Main");
  
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
  
  printf("Userfaultfd support: %s\n", uffd_is_supported() ? "true" : "false");
  return 0;
}

#endif
