#ifndef _headers_1686829262_FluffyGC_descriptor_registry
#define _headers_1686829262_FluffyGC_descriptor_registry

// Registry for various type (Not arrays included)

struct type_registry {
  
};

int type_registry_init(struct type_registry* self);
void type_registry_cleanup(struct type_registry* self);

struct descriptor* type_registry_get(struct type_registry* self, const char* path);

#endif

