#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <threads.h>

#include "marker.h"
#include "../heap.h"
#include "../region.h"
#include "../descriptor.h"

static void mark(const struct gc_marker_args ctx);
static void markNormalObject(const struct gc_marker_args ctx) {
  struct descriptor* desc = ctx.objectInfo->typeSpecific.normal.desc;
  
  // Opaque object
  if (!desc)
    return;

  for (int i = 0; i < desc->numFields; i++) {
    struct descriptor_field* field = &desc->fields[i];
    if (ctx.ignoreWeak && field->strength == REFERENCE_STRENGTH_WEAK)
      continue;
    if (ctx.ignoreSoft && field->strength == REFERENCE_STRENGTH_SOFT)
      continue;

    void* ptr;
    region_read(ctx.objectInfo->regionRef->owner, ctx.objectInfo->regionRef, field->offset, &ptr, sizeof(void*));
    if (!ptr)
      continue;
    
    struct region_reference* ref = heap_get_region_ref(ctx.objectInfo->owner, ptr);
    assert(ref);
    struct object_info* objectInfo = heap_get_object_info(ctx.objectInfo->owner, ref);
    assert(objectInfo);
     
    if (ref->owner != ctx.onlyIn)
      continue;

    // Add ref count as this strong reference
    if (field->strength == REFERENCE_STRENGTH_STRONG)
      atomic_fetch_add(&objectInfo->strongRefCount, 1);
    //printf("[Marking: Ref: %p] Name: '%s' (offset: %zu  ptr: %p)\n", ctx.objectInfo->regionRef, desc->fields[i].name, offset, ptr);
    
    mark(gc_marker_builder()
        ->copy_from(ctx)
        ->object_info(objectInfo)
        ->build()
    );
  }
}

static void markArrayObject(const struct gc_marker_args ctx) {
  enum reference_strength strength = ctx.objectInfo->typeSpecific.array.strength;
  if (ctx.ignoreWeak && strength == REFERENCE_STRENGTH_WEAK)
    return;
  if (ctx.ignoreSoft && strength == REFERENCE_STRENGTH_SOFT)
    return;

  void** array = ctx.objectInfo->regionRef->data;
  for (int i = 0; i < ctx.objectInfo->typeSpecific.array.size; i++) {
    if (!array[i])
      continue;

    struct region_reference* ref = heap_get_region_ref(ctx.objectInfo->owner, array[i]);
    assert(ref);
    struct object_info* objectInfo = heap_get_object_info(ctx.objectInfo->owner, ref);
   
    assert(objectInfo);
    
    if (ref->owner != ctx.onlyIn)
      continue;
    
    // Add ref count as this strong reference
    if (strength == REFERENCE_STRENGTH_STRONG)
      atomic_fetch_add(&objectInfo->strongRefCount, 1);

    //printf("[Marking: Ref: %p] Name: '%s' (offset: %zu  ptr: %p)\n", ctx.objectInfo->regionRef, desc->fields[i].name, offset, ptr);
    
    mark(gc_marker_builder()
        ->copy_from(ctx)
        ->object_info(objectInfo)
        ->build()
    );
  } 
}

static void mark(const struct gc_marker_args ctx) {
  if (ctx.objectInfo->regionRef->owner == ctx.onlyIn || ctx.onlyIn == NULL)
    if (atomic_exchange(&ctx.objectInfo->isMarked, true) == true)
      return;

  switch (ctx.objectInfo->type) {
    case OBJECT_TYPE_NORMAL:
      markNormalObject(ctx);
      break;
    case OBJECT_TYPE_ARRAY:
      markArrayObject(ctx);
      break;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }
}

void gc_marker_mark(struct gc_marker_args args) {
  mark(args);
}

static thread_local struct gc_marker_builder_struct builder = {
  .data = {},
  
  BUILDER_SETTER(builder, struct gc_state*, gcState, gc_state),
  BUILDER_SETTER(builder, struct object_info*, objectInfo, object_info),
  BUILDER_SETTER(builder, struct region*, onlyIn, only_in),
  BUILDER_SETTER(builder, bool, ignoreSoft, ignore_soft),
  BUILDER_SETTER(builder, bool, ignoreWeak, ignore_weak),
  
  .copy_from = ^struct gc_marker_builder_struct* (struct gc_marker_args data) {
    builder.data = data;
    return &builder;
  },

  .build = ^struct gc_marker_args () {
    return builder.data;
  }
};

struct gc_marker_builder_struct* gc_marker_builder() {
  struct gc_marker_args tmp = {};
  builder.data = tmp;
  return &builder;
}


