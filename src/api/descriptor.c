#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "concurrency/mutex.h"
#include "object/object.h"
#include "pre_code.h"

#include "concurrency/rwlock.h"
#include "context.h"
#include "api/type_registry.h"
#include "object/descriptor.h"
#include "managed_heap.h"
#include "FluffyHeap.h"
#include "util/list_head.h"
#include "vec.h"

typedef vec_t(struct descriptor*) descriptor_stack;

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

static int loadDescriptor(struct descriptor_loader_context* loader, const char* name, struct descriptor** result) {
  int ret = 0;
  struct descriptor* new = NULL;
  // Descriptor for the field haven't created invoke loader
  if (!(new = descriptor_new())) { 
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
    descriptor_free(new);
  
  *result = ret >= 0 ? new : NULL;
  return ret;
}

static int process(struct descriptor_loader_context* loader, struct descriptor* current) {
  fh_descriptor_param* param = &current->api.param;
  int ret = 0;
  
  if ((ret = type_registry_add_nolock(managed_heap_current->api.registry, current)) < 0)
    return ret;
  
  static enum object_type typeMapping[FH_TYPE_COUNT] = {
    [FH_TYPE_NORMAL] = OBJECT_NORMAL,
    [FH_TYPE_ARRAY] = -1 // -1 as marker value that its invalid
  };
  
  current->alignment = param->alignment;
  current->objectSize = param->size;
  current->objectType = typeMapping[param->type];
  BUG_ON(current->objectType < 0);
  
  static enum reference_strength strengthMapping[FH_REF_COUNT] = {
    [FH_REF_STRONG] = REFERENCE_STRONG,
    [FH_REF_PHANTOM] = -1,
    [FH_REF_WEAK] = -1,
    [FH_REF_SOFT] = -1,
  };
  
  for (int i = 0; param->fields[i].name; i++) {
    fh_descriptor_field* field = &param->fields[i];
    struct descriptor_field convertedField = (struct descriptor_field) {
      .offset = field->offset,
      .strength = strengthMapping[field->strength]
    };
    
    struct descriptor* dataType = type_registry_get_nolock(managed_heap_current->api.registry, field->dataType);
    if (dataType)
      goto descriptor_found;
    
    if ((ret = loadDescriptor(loader, field->dataType, &dataType)) < 0)
      goto failure;
    
    BUG_ON(dataType == NULL);
descriptor_found:
    convertedField.name = dataType->name;
    convertedField.dataType = dataType;
    
    if (vec_push(&current->fields, convertedField) < 0) {
      ret = -ENOMEM;
      goto failure;
    }
  }
  
failure:
  return ret;
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_define_descriptor(__FLUFFYHEAP_NONNULL(const char*) name, __FLUFFYHEAP_NONNULL(fh_descriptor_param*) parameter, bool dontInvokeLoader) {
  enterClassLoaderExclusive();
  
  descriptor_stack stack = {};
  struct descriptor_loader_context loader = {
    .appLoader = !dontInvokeLoader ? managed_heap_current->api.descriptorLoader : NULL
  };
  list_head_init(&loader.newlyAddedDescriptor);
  list_head_init(&loader.stack);
  
  int ret = 0;
  struct descriptor* newDescriptor = descriptor_new();
  if (!newDescriptor)
    return NULL;
  
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
    struct descriptor* current = list_first_entry(&loader.stack, struct descriptor, api.list);
    
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
    struct descriptor* current = list_entry(currentEntry, struct descriptor, api.list);
    list_del(currentEntry);
    
    if (ret < 0) {
      type_registry_remove_nolock(managed_heap_current->api.registry, current);
      descriptor_free(current);
    } else {
      descriptor_init(current);
      printf("[DescriptorLoader] Descriptor '%s' was loaded\n", current->name);
    }
  }
  
  vec_deinit(&stack);
  
  rwlock_unlock(&managed_heap_current->api.registry->lock);
  context_unblock_gc();
  
  exitClassLoaderExclusive();
  
  if (ret < 0) {
    printf("[DescriptorLoader] Failed loading '%s'\n", name);
    newDescriptor = NULL;
  }
  return EXTERN(newDescriptor);
}

static struct descriptor* getDescriptor(const char* name) {
  context_block_gc();
  rwlock_rdlock(&managed_heap_current->api.registry->lock);
  
  struct descriptor* desc = type_registry_get_nolock(managed_heap_current->api.registry, name);
  if (!desc)
    descriptor_acquire(desc);
  
  rwlock_unlock(&managed_heap_current->api.registry->lock);
  context_unblock_gc();
  
  return desc;
}

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_get_descriptor(__FLUFFYHEAP_NONNULL(const char*) name, bool dontInvokeLoader) {
  struct descriptor* desc = getDescriptor(name);
  
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
    if (ret >= 0)
      desc = INTERN(fh_define_descriptor(name, &param, dontInvokeLoader));
    exitClassLoaderExclusive();
    
    if (ret < 0)
      goto failure;
  }
false_negative:
failure:
  return EXTERN(desc);
}

__FLUFFYHEAP_EXPORT void fh_release_descriptor(__FLUFFYHEAP_NULLABLE(fh_descriptor*) desc) {
  descriptor_release(INTERN(desc));
}
