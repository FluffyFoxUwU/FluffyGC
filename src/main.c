#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "FluffyHeap.h"
#include "bug.h"
#include "hook/hook.h"
#include "mods/dma.h"

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

HOOK_TARGET(int, foo, int, arg1, int, arg2) {
  printf("foo: Real one called, returning %d\n", arg1 * arg2);
  return arg1 * arg2;
}

HOOK_FUNCTION(static, int, foo_hook_head, int, arg1, int, arg2) {
  int ret = 0;
  if (arg1 == arg2) {
    printf("foo_head: Special combination detected, overriding\n");
    ret = 8086;
    ci->action = HOOK_RETURN;
  } else {
    ci->action = HOOK_CONTINUE;
  }
  return ret;
}

int hookExample() {
  hook_init();
  
  hook_register(foo, HOOK_HEAD, foo_hook_head);
  
  printf("Main got %d\n", foo(5, 6));
  printf("Main got %d\n", foo(87, 87));
  
  hook_unregister(foo, HOOK_HEAD, foo_hook_head);
  return EXIT_SUCCESS;
}

HOOK_FUNCTION(static, int, foo_hook_tail, int, arg1, int, arg2) {
  printf("foo_tail: I'm called after\n");
  ci->action = HOOK_CONTINUE;
  return 0;
}

HOOK_FUNCTION(static, int, foo_hook_invoke, int, arg1, int, arg2) {
  printf("foo_invoke: I'm taking control!\n");
  ci->action = HOOK_RETURN;
  return arg2;
}

int main2() {
  if (1)
    return hookExample();
  
  if (hook_init() < 0) {
    puts("Hook_init failed");
    goto hook_init_fail;
  }
  
  if (hook_register(foo, HOOK_HEAD, foo_hook_head) < 0) {
    puts("Head register failed");
    goto head_registration_fail;
  }
  
  if (hook_register(foo, HOOK_TAIL, foo_hook_tail) < 0) {
    puts("Tail register failed");
    goto tail_registration_fail;
  }
  
  if (hook_register(foo, HOOK_INVOKE, foo_hook_invoke) < 0) {
    puts("Invoke register failed");
    goto invoke_registration_fail;
  }
  
  printf("Main got %d\n", foo(5, 6));
  
  hook_unregister(foo, HOOK_INVOKE, foo_hook_invoke);
invoke_registration_fail:
  hook_unregister(foo, HOOK_TAIL, foo_hook_tail);
tail_registration_fail:
  hook_unregister(foo, HOOK_HEAD, foo_hook_head);
head_registration_fail:
hook_init_fail:
  return EXIT_SUCCESS;
}

int main23() {
  size_t sizes[] = {
    64 * 1024 * 1024
  };
  
  fh_param param = {
    .hint = FH_GC_HIGH_THROUGHPUT,
    .generationCount = 1,
    .generationSizes = sizes
  };
  
  int ret = fh_enable_mod(FH_MOD_DMA, FH_MOD_DMA_ATOMIC | FH_MOD_DMA_NONBLOCKING);
  if (ret < 0) {
    errno = -ret;
    perror("fh_enable_mod");
    return EXIT_FAILURE;
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
  fh_descriptor* fluffDesc = fh_get_descriptor("fox.fluffygc.Fluff", false);
  
  fh_object* obj = fh_alloc_object(fluffDesc);
  
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
    fh_array* array = fh_alloc_array(fluffDesc, 5);
    fh_array_set_element(array, 0, obj);
    fh_object* readVal = fh_array_get_element(array, 0);
    printf("[Main] Array[0] is %ssame what just written\n", fh_object_is_alias(obj, readVal) ? "" : "not ");
    
    const fh_type_info* info = fh_object_get_type_info(FH_CAST_TO_OBJECT(array));
    printf("[Main] Array is %zu has entries long\n", info->info.refArray->length);
    fh_object_put_type_info(FH_CAST_TO_OBJECT(array), info);
    
    fh_del_ref((fh_object*) array);
    fh_del_ref((fh_object*) readVal);
  }
  
  fh_del_ref(obj);
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
    printf("UwU Result 0x%llx\n", result);
    fh_del_ref(obj);
  }
  
  // Cleaning up
  fh_detach_thread(heap);
  fh_free(heap);
  return EXIT_SUCCESS;
}
