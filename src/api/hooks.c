#include "hooks.h"
#include "object/descriptor.h"

void api_on_descriptor_free(struct descriptor* desc) {
  free((void*) desc->name);
}
