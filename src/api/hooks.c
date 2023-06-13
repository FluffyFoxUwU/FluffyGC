#include "hooks.h"
#include "vec.h"
#include "object/descriptor.h"

void api_on_descriptor_free(struct descriptor* desc) {
  int i;
  struct descriptor_field* field;
  vec_foreach_ptr(&desc->fields, field, i)
    free((void*) field->name);
  
  vec_deinit(&desc->fields);
  free((void*) desc->name);
}
