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
  
  fh_set_current(current);
  
  fh_set_current(NULL);
  
  fh_free_context(heap, current);
  fh_detach_thread(heap);
  
  fh_free(heap);
  return EXIT_SUCCESS;
}
