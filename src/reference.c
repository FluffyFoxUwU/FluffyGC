#include <stdlib.h>

#include "reference.h"
#include "root.h"

static struct reference* commonNew() {
  return malloc(sizeof(struct reference));
}

struct reference* reference_new_strong(struct region_reference* ref) {
  struct reference* self = commonNew();
  if (!self)
    return NULL;

  self->type = REFERENCE_STRONG;
  self->data.strong = ref;
  return self;
}
struct reference* reference_new_root(struct root_reference* ref) {
  struct reference* self = commonNew();
  if (!self)
    return NULL;

  self->type = REFERENCE_ROOT;
  self->data.root = ref;
  return self;
}

struct region_reference* reference_get(struct reference* self) {
  switch (self->type) {
    case REFERENCE_STRONG:
      return self->data.strong;
    case REFERENCE_ROOT:
      return root_ref_get(self->data.root);
    case REFERENCE_INVALID:
      return NULL;
  }
  abort();
}

struct root_reference* reference_get_root(struct reference* self) {
  if (self->type == REFERENCE_ROOT)
    return self->data.root;

  return NULL;
}

void reference_free(struct reference* self) {
  free(self);
}


