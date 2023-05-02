#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "context.h"

int fuzzing_root_refs(const void* data, size_t size) {
  if (size < sizeof(uint16_t))
    return 0;
  
  const uint16_t* dataAsUint16 = data; 
  
  uint16_t refLookupCount = dataAsUint16[0];
  size_t count = size / sizeof(uint16_t);
  if (refLookupCount < 1)
    return 0;
  
  struct context* context = context_new();
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
  return 0;
}