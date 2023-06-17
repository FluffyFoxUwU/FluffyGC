#include "hooks.h"
#include "api/type_registry.h"
#include "managed_heap.h"
#include "object/object_descriptor.h"

void api_on_object_descriptor_free(struct object_descriptor* desc) {
  type_registry_remove(managed_heap_current->api.registry, desc);
  free((void*) desc->name);
}
