#include <stdio.h>
#include <stdlib.h>

#include "FluffyHeap.h"

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
  
  fh_attach_thread(heap);
  fh_context* current = fh_new_context(heap);
  
  struct test_type {
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
  
  fh_descriptor_field fields[] = {
    FH_FIELD(struct test_type, fox, "fox.fluffygc.Test", FH_REF_STRONG),
    FH_FIELD_END()
  };
  
  fh_descriptor_param descParam = {
    .alignment = alignof(struct test_type),
    .type = FH_TYPE_NORMAL,
    .fields = fields,
    .size = sizeof(struct test_type)
  };
  
  fh_set_current(current);
  fh_descriptor* desc = fh_define_descriptor("fox.fluffygc.Test", &descParam);
  
  fh_object* obj = fh_alloc_object(desc);
  struct test_type data;
  fh_object_read_data(obj, &data, 0, sizeof(data));
  data.a = "C string test UwU";
  fh_object_write_data(obj, &data, 0, sizeof(data));
  fh_object_read_data(obj, &data, 0, sizeof(data));
  printf("Got: %s\n", data.a);
  
  fh_object_write_ref(obj, offsetof(struct test_type, fox), obj);
  
  fh_object* readVal = fh_object_read_ref(obj, offsetof(struct test_type, fox));
  printf("Object is %ssame object\n", fh_object_is_alias(obj, readVal) ? "" : "not ");
  fh_del_ref(readVal);
  
  fh_del_ref(obj);
  
  fh_release_descriptor(desc);
  fh_set_current(NULL);
  
  fh_free_context(heap, current);
  fh_detach_thread(heap);
  
  fh_free(heap);
  return EXIT_SUCCESS;
}
