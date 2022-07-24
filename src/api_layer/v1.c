#include <stdbool.h>
#include <stdlib.h>

#include "fluffygc/v1.h"

#include "heap.h"
#include "descriptor.h"

#define CAST(val) (_Generic((val), \
    struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec*: \
      (struct heap*) (val), \
    struct _2aaee2d4_2f50_4685_97f4_63296ae1f585*: \
      (struct descriptor*) (val), \
    struct heap*: \
      (struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec*) (val), \
    struct descriptor*: \
      (struct _2aaee2d4_2f50_4685_97f4_63296ae1f585*) (val) \
    ))

FLUFFYGC_DECLARE(fluffygc_state*, fluffygc_new, 
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold) {
  return CAST(heap_new(youngSize, 
                    oldSize, 
                    metaspaceSize, 
                    localFrameStackSize, 
                    concurrentOldGCthreshold));
}

FLUFFYGC_DECLARE(void, fluffygc_free,
    fluffygc_state* self) {
  heap_free(CAST(self));
}

FLUFFYGC_DECLARE(fluffygc_descriptor*, fluffygc_descriptor_new,
    fluffygc_state* self,
    fluffygc_descriptor_args* arg) {
  if (!heap_is_attached(CAST(self)))
    abort();

  struct descriptor_field* fields = calloc(0, sizeof(*fields));
  for (int i = 0; i < arg->fieldCount; i++) {
    fields[i].name = arg->fields[i].name;
    fields[i].offset = arg->fields[i].offset;
    fields[i].type = (enum field_type) arg->fields[i].type;
  }
  
  struct descriptor_typeid id = {
    .name = arg->name,
    .ownerID = arg->ownerID,
    .typeID = arg->typeID
  };
  struct descriptor* desc = heap_descriptor_new(CAST(self), id, arg->objectSize, arg->fieldCount, fields);
  free(fields);
  return CAST(desc);
}


