#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "FluffyHeap.h"
#include "bug.h"
#include "hook/hook.h"
#include "mods/dma.h"
#include "rcu/rcu.h"
#include "util/util.h"

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

void doTestNormal() {
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
    fh_object* obj = fh_alloc_object(fluffDesc);
    
    fh_array* array = fh_alloc_array(fluffDesc, 5);
    fh_array_set_element(array, 0, obj);
    fh_object* readVal = fh_array_get_element(array, 0);
    printf("[Main] Array[0] is %ssame what just written\n", fh_object_is_alias(obj, readVal) ? "" : "not ");
    
    // Triger warning (or abort) intentionally
    fh_array_calc_offset(array, 999);
    
    const fh_type_info* info = fh_object_get_type_info(FH_CAST_TO_OBJECT(array));
    printf("[Main] Array is %zu has entries long\n", info->info.refArray->length);
    printf("[Main] Array is %zu has entries according to API call\n", fh_array_get_length(array));
    fh_object_put_type_info(FH_CAST_TO_OBJECT(array), info);
    
    printf("[Main] Not real array has %zu entries according to API call\n", fh_array_get_length(FH_CAST_TO_ARRAY(obj)));
    
    fh_del_ref((fh_object*) array);
    fh_del_ref((fh_object*) readVal);
    fh_del_ref((fh_object*) obj);
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
}

struct an_rcu_using_struct {
  struct rcu_head rcuHead;
  
  int uwu;
};

struct rcu_struct rcuProtected;
void* worker(void*) {
  uint64_t iteration = 0;
  pthread_t selfThread = pthread_self();
  while (1) {
    rcu_read_lock(&rcuProtected);
    struct an_rcu_using_struct* result = container_of(rcu_read(&rcuProtected), struct an_rcu_using_struct, rcuHead);
    fprintf(stderr, "Reader: %d -> %20ld (Thread: %20ld)\r", result->uwu, iteration, selfThread);
    rcu_read_unlock(&rcuProtected, &result->rcuHead);
    
    pthread_testcancel();
    iteration++;
  }
  return NULL;
}

void freeRcuData(struct rcu_head* head) {
  struct an_rcu_using_struct* self = container_of(head, struct an_rcu_using_struct, rcuHead);
  free(self);
}

void doTestRCU() {
  rcu_init(&rcuProtected);
  pthread_t new, new2;
  
  {
    struct an_rcu_using_struct* currentData = malloc(sizeof(*currentData));
    *currentData = (struct an_rcu_using_struct) {};
    rcu_exchange_and_synchronize(&rcuProtected, &currentData->rcuHead);
  }
  
  pthread_create(&new, NULL, worker, NULL);
  pthread_create(&new2, NULL, worker, NULL);
  
  for (int i = 0; i < 5; i++) {
    fprintf(stderr, "\033[2KWriter start writing %d into data\n", i);
    struct an_rcu_using_struct* newCopy = malloc(sizeof(*newCopy));
    
    // Create copy
    {
      rcu_read_lock(&rcuProtected);
      struct an_rcu_using_struct* current = container_of(rcu_read(&rcuProtected), struct an_rcu_using_struct, rcuHead);
      rcu_memcpy(&newCopy->rcuHead, &current->rcuHead, offsetof(struct an_rcu_using_struct, rcuHead), sizeof(*newCopy));
      rcu_read_unlock(&rcuProtected, &current->rcuHead);
    }
    
    // Update
    newCopy->uwu = i;
    
    // Write, synchronize and clean old instance
    struct rcu_head* old = rcu_exchange_and_synchronize(&rcuProtected, &newCopy->rcuHead);
    free(container_of(old, struct an_rcu_using_struct, rcuHead));
    
    fprintf(stderr, "Writer written %d into data\n", i);
    sleep(1);
  }
  
  pthread_cancel(new);
  pthread_cancel(new2);
  pthread_join(new, NULL);
  pthread_join(new2, NULL);
  rcu_cleanup(&rcuProtected);
}

int main2() {
  hook_init();
  
  doTestRCU();
  
  return EXIT_SUCCESS;
}
