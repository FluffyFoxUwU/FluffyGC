#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "bug.h"
#include "concurrency/mutex.h"
#include "object/object.h"
#include "pre_code.h"

#include "object/object_descriptor.h"
#include "concurrency/rwlock.h"
#include "context.h"
#include "api/type_registry.h"
#include "object/descriptor.h"
#include "managed_heap.h"
#include "FluffyHeap.h"
#include "util/list_head.h"
#include "util/util.h"
#include "vec.h"

typedef vec_t(struct object_descriptor*) descriptor_stack;

struct descriptor_loader_context {
  struct list_head stack;
  struct list_head newlyAddedDescriptor;
  fh_descriptor_loader appLoader;
};

static thread_local int exclusiveCount = 0;
static void enterClassLoaderExclusive() {
  if (exclusiveCount == 0)
    mutex_lock(&managed_heap_current->api.descriptorLoaderSerializationLock);
  exclusiveCount++;
}

static void exitClassLoaderExclusive() {
  if (exclusiveCount == 1)
    mutex_unlock(&managed_heap_current->api.descriptorLoaderSerializationLock);
  exclusiveCount--;
}

static int loadDescriptor(struct descriptor_loader_context* loader, const char* name, struct object_descriptor** result) {
  int ret = 0;
  struct object_descriptor* new = NULL;
  // Descriptor for the field haven't created invoke loader
  if (!(new = object_descriptor_new())) { 
    ret = -ENOMEM;
    goto failure;
  }
  
  if ((new->name = strdup(name)) < 0) {
    ret = -ENOMEM;
    goto failure;
  }
  
  rwlock_unlock(&managed_heap_current->api.registry->lock);
  context_unblock_gc();
  if (loader->appLoader) {
    printf("[DescriptorLoader] Calling Application's loader for '%s'\n", name);
    ret = loader->appLoader(name, managed_heap_current->api.udata, &new->api.param);
    printf("[DescriptorLoader] Loaded '%s' result: %d\n", name, ret);
  } else {
    printf("[DescriptorLoader] Loader not present or disabled during loading of '%s'\n", name);
    ret = -ESRCH;
  }
  context_block_gc();
  rwlock_wrlock(&managed_heap_current->api.registry->lock);
  if (ret < 0)
    goto failure;
  
  list_add(&new->api.list, &loader->stack);
failure:
  if (ret < 0)
    object_descriptor_free(new);
  
  *result = ret >= 0 ? new : NULL;
  return ret;
}

static int process(struct descriptor_loader_context* loader, struct object_descriptor* current) {
  fh_descriptor_param* param = &current->api.param;
  int ret = 0;
  
  if ((ret = type_registry_add_nolock(managed_heap_current->api.registry, current->parent)) < 0)
    return ret;
  
  current->alignment = param->alignment;
  current->objectSize = param->size;
  
  static enum reference_strength strengthMapping[FH_REF_COUNT] = {
    [FH_REF_STRONG] = REFERENCE_STRONG,
    [FH_REF_PHANTOM] = -1,
    [FH_REF_WEAK] = -1,
    [FH_REF_SOFT] = -1,
  };
  
  for (int i = 0; param->fields[i].name; i++) {
    fh_descriptor_field* field = &param->fields[i];
    struct object_descriptor_field convertedField = (struct object_descriptor_field) {
      .offset = field->offset,
      .strength = strengthMapping[field->strength]
    };
    
    struct descriptor* dataType = type_registry_get_nolock(managed_heap_current->api.registry, field->dataType);
    if (dataType)
      goto descriptor_found;
    
    struct object_descriptor* newlyLoadedDesriptor;
    if ((ret = loadDescriptor(loader, field->dataType, &newlyLoadedDesriptor)) < 0)
      goto failure;
    
    dataType = newlyLoadedDesriptor->parent;
descriptor_found:
    convertedField.name = descriptor_get_name(dataType);
    convertedField.dataType = dataType;
    
    if (vec_push(&current->fields, convertedField) < 0) {
      ret = -ENOMEM;
      goto failure;
    }
  }
  
failure:
  return ret;
}

__FLUFFYHEAP_EXPORT int fh_define_descriptor(__FLUFFYHEAP_NONNULL(const char*) name, __FLUFFYHEAP_NONNULL(fh_descriptor_param*) parameter, bool dontInvokeLoader) {
  enterClassLoaderExclusive();
  
  descriptor_stack stack = {};
  struct descriptor_loader_context loader = {
    .appLoader = !dontInvokeLoader ? managed_heap_current->api.descriptorLoader : NULL
  };
  list_head_init(&loader.newlyAddedDescriptor);
  list_head_init(&loader.stack);
  
  int ret = 0;
  struct object_descriptor* newDescriptor = object_descriptor_new();
  if (!newDescriptor)
    return -ENOMEM;
  
  context_block_gc();
  rwlock_wrlock(&managed_heap_current->api.registry->lock);
  vec_init(&stack);
  
  newDescriptor->api.param = *parameter;
  if (!(newDescriptor->name = strdup(name))) {
    ret = -ENOMEM;
    goto failure;
  }
  
  list_add(&newDescriptor->api.list, &loader.stack);
  while (!list_is_empty(&loader.stack)) {
    struct object_descriptor* current = list_first_entry(&loader.stack, struct object_descriptor, api.list);
    
    // Add so later can find it
    list_del(&current->api.list);
    list_add(&current->api.list, &loader.newlyAddedDescriptor);
    
    if ((ret = process(&loader, current)) < 0)
      goto failure;
  }
  
failure:;
  struct list_head* currentEntry;
  struct list_head* next;
  list_for_each_safe(currentEntry, next, &loader.newlyAddedDescriptor) {
    struct object_descriptor* current = list_entry(currentEntry, struct object_descriptor, api.list);
    list_del(currentEntry);
    
    if (ret < 0) {
      type_registry_remove_nolock(managed_heap_current->api.registry, current->parent);
      object_descriptor_free(current);
    } else {
      object_descriptor_init(current);
      list_add(&current->parent->list, &managed_heap_current->descriptorList);
      printf("[DescriptorLoader] Descriptor '%s' was loaded\n", current->name);
    }
  }
  
  vec_deinit(&stack);
  
  rwlock_unlock(&managed_heap_current->api.registry->lock);
  context_unblock_gc();
  
  exitClassLoaderExclusive();
  
  if (ret < 0)
    printf("[DescriptorLoader] Failed loading '%s'\n", name);
  return ret;
}

static struct object_descriptor* getDescriptor(const char* name) {
  context_block_gc();
  rwlock_rdlock(&managed_heap_current->api.registry->lock);
  
  struct descriptor* desc = type_registry_get_nolock(managed_heap_current->api.registry, name);
  if (desc)
    descriptor_acquire(desc);
  
  rwlock_unlock(&managed_heap_current->api.registry->lock);
  context_unblock_gc();
  
  BUG_ON(desc->type != OBJECT_NORMAL);
  return desc->info.normal;
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_get_descriptor(__FLUFFYHEAP_NONNULL(const char*) name, bool dontInvokeLoader) {
  // Those are special and must not be accessed
  if (util_prefixed_by("fox.fluffyheap.marker.", name))
    return NULL;
  
  struct object_descriptor* desc = getDescriptor(name);
  
  if (!desc && !dontInvokeLoader) {
    enterClassLoaderExclusive();
    
    // This thread may have been blocked by loader which potentially
    // loads what this thread needs so recheck
    if ((desc = getDescriptor(name)) != NULL) {
      exitClassLoaderExclusive();
      goto false_negative;
    }
    
    fh_descriptor_param param;
    int ret = managed_heap_current->api.descriptorLoader(name, managed_heap_current->api.udata, &param);
    if (ret >= 0) {
      ret = fh_define_descriptor(name, &param, dontInvokeLoader);
      if (ret >= 0)
        desc = getDescriptor(name);
    }
    exitClassLoaderExclusive();
    
    if (ret < 0)
      goto failure;
  }
  
false_negative:
failure:
  return EXTERN(desc->parent);
}

__FLUFFYHEAP_EXPORT void fh_release_descriptor(__FLUFFYHEAP_NULLABLE(fh_descriptor*) desc) {
  descriptor_release(INTERN(desc));
}
