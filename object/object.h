#ifndef UWU_A16FF0D3_538A_458B_B923_6E9AF954B71F_UWU
#define UWU_A16FF0D3_538A_458B_B923_6E9AF954B71F_UWU

#include <stddef.h>

#include "object/descriptor.h"

struct object {
  struct descriptor* desc;
  
  // User data come after this
  alignas(max_align_t) char data[];
};

void object_init(struct object* self, struct descriptor* desc);

// Returns the value of last successful walker
// where negative considered a failure and return early
int object_walk(struct object* self, int (^walker)(struct object*));

struct object* object_read_ref(struct object* self, size_t fieldOffset);
void object_write_ref(struct object* self, size_t fieldOffset, struct object* obj);

#endif
