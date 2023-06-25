#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "FluffyHeap.h"
#include "bug.h"

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
  .alignment = alignof(struct test_type2),
  .fields = fields2,
  .size = sizeof(struct test_type2)
};

static int loader(const char* name, void* udata, fh_descriptor_param* param) {
  if (strcmp(name, "fox.fluffygc.Test2") == 0) {
    *param = descParam2;
    return 0;
  }
  
  return -ESRCH;
}

int main2() {
  size_t sizes[] = {
    64 * 1024 * 1024
  };
  
  fh_param param = {
    .hint = FH_GC_HIGH_THROUGHPUT,
    .generationCount = 1,
    .generationSizes = sizes
  };
  
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
    .alignment = alignof(struct fluff),
    .fields = fields,
    .size = sizeof(struct fluff)
  };
  
  fh_define_descriptor("fox.fluffygc.Fluff", &descParam, false);
  //fh_define_descriptor("fox.fluffygc.Test2", &descParam2, false);
  fh_descriptor* desc = fh_get_descriptor("fox.fluffygc.Fluff", false);
  
  fh_object* obj = fh_alloc_object(desc);
  
  // Object test
  {
    struct fluff data;
    fh_object_read_data(obj, &data, 0, sizeof(data));
    data.a = "C string test UwU";
    fh_object_write_data(obj, &data, 0, sizeof(data));
    fh_object_read_data(obj, &data, 0, sizeof(data));
    printf("[Main] Got: %s\n", data.a);
    
    fh_object_write_ref(obj, offsetof(struct fluff, fox), obj);
    fh_object_write_ref(obj, offsetof(struct fluff, any), obj);
    
    fh_object* readVal = fh_object_read_ref(obj, offsetof(struct fluff, fox));
    printf("[Main] Object is %ssame object\n", fh_object_is_alias(obj, readVal) ? "" : "not ");
    fh_del_ref(readVal);
  }
  
  // Array test
  {
    fh_array* array = fh_alloc_array(desc, 5);
    fh_array_set_element(array, 0, obj);
    fh_object* readVal = fh_array_get_element(array, 0);
    printf("[Main] Array[0] is %ssame what just written\n", fh_object_is_alias(obj, readVal) ? "" : "not ");
    
    fh_del_ref((fh_object*) array);
    fh_del_ref((fh_object*) readVal);
  }
  
  fh_del_ref(obj);
  fh_release_descriptor(desc);
  
  // Cleaning up
  fh_detach_thread(heap);
  fh_free(heap);
  return EXIT_SUCCESS;
}
