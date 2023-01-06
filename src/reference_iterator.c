#include <threads.h>
#include <stdlib.h>

#include "builder.h"
#include "reference_iterator.h"
#include "heap.h"
#include "region.h"
#include "descriptor.h"

static void processOne(const struct reference_iterator_args ctx, size_t offset, void* ptr) {
  if (!ptr)
    return;

  struct region_reference* ref = heap_get_region_ref_from_ptr(ctx.heap, ptr);
  struct object_info* info = NULL;

  // If unrecognized pass NULL for `info`  
  if (ref) {
    info = heap_get_object_info(ctx.heap, ref);
    assert(info);
  }

  ctx.consumer(info, ptr, offset);
}

static void iterateNormalObject(const struct reference_iterator_args ctx) {
  struct descriptor* desc = ctx.object->typeSpecific.normal.desc;
  struct region_reference* ref = ctx.object->regionRef;
  
  // Opaque object
  if (!desc)
    return;

  for (int i = 0; i < desc->numFields; i++) {
    struct descriptor_field* field = &desc->fields[i];
    size_t offset = field->offset;
    void* ptr;
    region_read(ref->owner, ref, offset, &ptr, sizeof(void*));
    
    if (ctx.ignoreSoft && field->strength == REFERENCE_STRENGTH_SOFT)
      continue;
    if (ctx.ignoreWeak && field->strength == REFERENCE_STRENGTH_WEAK)
      continue;
    if (ctx.ignoreStrong && field->strength == REFERENCE_STRENGTH_STRONG)
      continue;

    processOne(ctx, offset, ptr);
  }
}

static void iterateArrayObject(const struct reference_iterator_args ctx) {
  void** array = ctx.object->regionRef->untypedRawData;
  enum reference_strength strength = ctx.object->typeSpecific.array.strength;

  if (ctx.ignoreSoft && strength == REFERENCE_STRENGTH_SOFT)
    return;
  if (ctx.ignoreWeak && strength == REFERENCE_STRENGTH_WEAK)
    return;
  if (ctx.ignoreStrong && strength == REFERENCE_STRENGTH_STRONG)
    return;

  for (int i = 0; i < ctx.object->typeSpecific.array.size; i++)
    processOne(ctx, sizeof(void*) * i, array[i]);
}

void reference_iterator_run(struct reference_iterator_args ctx) {
  assert(ctx.object->isValid);

  switch (ctx.object->type) {
    case OBJECT_TYPE_NORMAL:
      iterateNormalObject(ctx);
      break;
    case OBJECT_TYPE_ARRAY:
      iterateArrayObject(ctx);
      break;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }
}

thread_local struct reference_iterator_builder builder = {
  .data = {},

  BUILDER_SETTER(builder, reference_consumer_t, consumer, consumer),
  BUILDER_SETTER(builder, bool, ignoreSoft, ignore_soft),
  BUILDER_SETTER(builder, bool, ignoreWeak, ignore_weak),
  BUILDER_SETTER(builder, bool, ignoreStrong, ignore_strong),
  BUILDER_SETTER(builder, struct heap*, heap, heap),
  BUILDER_SETTER(builder, struct object_info*, object, object),

  .copy_from = ^struct reference_iterator_builder* (struct reference_iterator_args data) {
    builder.data = data;
    return &builder;
  },

  .build = ^struct reference_iterator_args () {
    return builder.data;
  }
};

struct reference_iterator_builder* reference_iterator_builder() {
  struct reference_iterator_args tmp = {};
  builder.data = tmp;
  return &builder;
}


