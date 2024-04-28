#include <stdatomic.h>
#include <stddef.h>

#include "object.h"
#include "descriptor.h"

void object_init(struct object* self, struct descriptor* desc) {
  self->desc = desc;
  for (size_t i = 0; i < self->desc->fieldCount; i++)
    atomic_store((_Atomic(struct object*)*) (void*) ((char*) self->data + self->desc->fields[i].offset), NULL);
}

int object_walk(struct object* self, int (^walker)(struct object*)) {
  int ret = 0;
  for (size_t i = 0; i < self->desc->fieldCount; i++) {
    struct object* ref = object_read_ref(self, self->desc->fields[i].offset);
    if ((ret = walker(ref)) < 0)
      break;
  }
  
  return ret;
}

struct object* object_read_ref(struct object* self, size_t fieldOffset) {
  return atomic_load((_Atomic(struct object*)*) (void*) ((char*) self->data + fieldOffset));
}

void object_write_ref(struct object* self, size_t fieldOffset, struct object* obj) {
  atomic_store((_Atomic(struct object*)*) (void*) ((char*) self->data + fieldOffset), obj);
}

