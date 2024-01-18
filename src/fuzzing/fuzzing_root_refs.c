#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "context.h"
#include "gc/gc.h"

int fuzzing_root_refs(const void* data, size_t size);
int fuzzing_root_refs(const void* data, size_t size) {
  if (size < sizeof(uint16_t))
    return 0;
  
  const uint16_t* dataAsUint16 = data; 
  
  uint16_t refLookupCount = dataAsUint16[0];
  size_t count = size / sizeof(uint16_t);
  if (refLookupCount < 1)
    return 0;
  
  gc_current = gc_new(GC_NOP_GC, 0);
  
  struct context* context = context_new(NULL);
  context_current = context;
  struct root_ref** refLookup = calloc(refLookupCount, sizeof(*refLookup));
  
  for (int i = 1; i < count; i++) {
    int index = dataAsUint16[i] % refLookupCount;
    if (refLookup[index] == NULL) {
      refLookup[index] = context_add_root_object(NULL);
    } else {
      context_remove_root_object(refLookup[index]);
      refLookup[index] = NULL;
    }
  }
  
  free(refLookup);
  context_free(context);
  gc_free(gc_current);
  return 0;
}
