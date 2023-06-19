#include <stdio.h>
#include <stdlib.h>

#include "descriptor.h"
#include "object/object.h"
#include "object_descriptor.h"
#include "panic.h"
#include "descriptor.h"
#include "util/refcount.h"

static struct descriptor* newCommon() {
  struct descriptor* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct descriptor) {};
  refcount_init(&self->usages);
  return self;
}

struct descriptor* descriptor_new_for_object_type(struct object_descriptor* desc) {
  struct descriptor* self = newCommon();
  if (!self)
    return NULL;
  
  self->type = OBJECT_NORMAL;
  self->info.normal = desc;
  return self;
}

void descriptor_free(struct descriptor* self) {
  if (!self)
    return;
  
  switch (self->type) {
    case OBJECT_NORMAL: 
      object_descriptor_free(self->info.normal);
      break;
    case OBJECT_UNMAKEABLE_STATIC: panic();
  }
  free(self);
}

void descriptor_for_each_offset(struct object* object, void (^iterator)(size_t offset)) {
  switch (object->movePreserve.descriptor->type) {
    case OBJECT_NORMAL: return object_descriptor_for_each_offset(object->movePreserve.descriptor->info.normal, object, iterator);
    case OBJECT_UNMAKEABLE_STATIC: panic();
  }
  panic();
}

static bool isCompatible(struct descriptor* a, struct descriptor* b) {
  if (a->type == OBJECT_UNMAKEABLE_STATIC) {
    switch (a->info.unmakeable->type) {
      case DESCRIPTOR_UNMAKEABLE_ANY_MARKER: return true;
    }
  }
  
  return a == b;
}

int descriptor_is_assignable_to(struct object* self, size_t offset, struct descriptor* b) {
  switch (self->movePreserve.descriptor->type) {
    case OBJECT_NORMAL: return isCompatible(object_descriptor_get_at(self->movePreserve.descriptor->info.normal, offset), b);
    case OBJECT_UNMAKEABLE_STATIC: panic();
  }
  panic();
}

void descriptor_acquire(struct descriptor* self) {
  refcount_acquire(&self->usages);
}

void descriptor_release(struct descriptor* self) {
  if (refcount_release(&self->usages))
    return;
  panic();
}

void descriptor_init_object(struct descriptor* self, struct object* obj) {
  switch (self->type) {
    case OBJECT_NORMAL: return object_descriptor_init_object(self->info.normal, obj);
    case OBJECT_UNMAKEABLE_STATIC: panic();
  }
  panic();
}

size_t descriptor_get_object_size(struct descriptor* self) {
  switch (self->type) {
    case OBJECT_NORMAL: return object_descriptor_get_object_size(self->info.normal);
    case OBJECT_UNMAKEABLE_STATIC: panic();
  }
  panic();
}

size_t descriptor_get_alignment(struct descriptor* self) {
  switch (self->type) {
    case OBJECT_NORMAL: return object_descriptor_get_alignment(self->info.normal);
    case OBJECT_UNMAKEABLE_STATIC: panic();
  }
  panic();
}

const char* descriptor_get_name(struct descriptor* self) {
  switch (self->type) {
    case OBJECT_NORMAL: return object_descriptor_get_name(self->info.normal);
    case OBJECT_UNMAKEABLE_STATIC: return self->info.unmakeable->name;
  }
  panic();
}
