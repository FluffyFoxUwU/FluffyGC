#include <stdbool.h>
#include <stdlib.h>

#include "fluffygc/v1.h"

#include "config.h"
#include "heap.h"
#include "descriptor.h"
#include "root.h"
#include "thread.h"

#if IS_ENABLED(CONFIG_API_ENABLE_V1)

#define CAST(val) (_Generic((val), \
    struct _7ca2fe74_2f67_4a70_a55d_ba9f6acb6f43*: \
      (struct root_reference*) (val), \
    struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec*: \
      (struct heap*) (val), \
    struct _431d5fb5_4c3f_45e2_9f89_59521ff5cb07*: \
      (struct heap*) (val), \
    struct _2aaee2d4_2f50_4685_97f4_63296ae1f585*: \
      (struct descriptor*) (val), \
     \
    struct root_reference*: \
      (struct _7ca2fe74_2f67_4a70_a55d_ba9f6acb6f43*) (val), \
    struct heap*: \
      (struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec*) (val), \
    struct descriptor*: \
      (struct _2aaee2d4_2f50_4685_97f4_63296ae1f585*) (val) \
    ))

static void checkIfInAttachedThread(struct heap* heap, const char* funcName) {
  if (heap_is_attached(heap))
    return;
  
  heap_report_printf(heap, "%s: Illegal API call on unattached thread", funcName);
  abort();
}

static struct thread* getThread(struct heap* heap) {
  return heap_get_thread_data(heap)->thread;
}

static void _apiAssert(struct heap* heap, bool cond, const char* func, const char* msg, const char* expr) {
  if (cond)
    return;

  heap_report_printf(heap, "%s: API assert failed: %s (Expression '%s')", func, msg, expr);
  abort();
}

#define apiAssert(state, expr, msg) _apiAssert(state, expr, __func__, msg, #expr)

FLUFFYGC_DECLARE(fluffygc_state*, new, 
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold) {
  return CAST(heap_new(youngSize, 
                    oldSize, 
                    metaspaceSize, 
                    localFrameStackSize, 
                    concurrentOldGCthreshold));
}

FLUFFYGC_DECLARE(void, free,
    fluffygc_state* self) {
  heap_free(CAST(self));
}

FLUFFYGC_DECLARE(fluffygc_descriptor*, descriptor_new,
    fluffygc_state* self,
    fluffygc_descriptor_args* arg) {
  checkIfInAttachedThread(CAST(self), __func__);

  struct descriptor_field* fields = calloc(arg->fieldCount, sizeof(*fields));
  for (int i = 0; i < arg->fieldCount; i++) {
    fields[i].name = arg->fields[i].name;
    fields[i].offset = arg->fields[i].offset;
    fields[i].isArray = arg->fields[i].isArray;
    fields[i].type = (enum field_type) arg->fields[i].type;
  }
  
  struct descriptor_typeid id = {
    .name = arg->name,
    .ownerID = arg->ownerID,
    .typeID = arg->typeID
  };
  struct descriptor* desc = heap_descriptor_new(CAST(self), id, arg->objectSize, arg->fieldCount, fields);
  free(fields);
  return CAST(desc);
}

FLUFFYGC_DECLARE(void, descriptor_delete, 
    fluffygc_state* self,
    fluffygc_descriptor* desc) {
  checkIfInAttachedThread(CAST(self), __func__);
  heap_descriptor_release(CAST(self), CAST(desc));
}

FLUFFYGC_DECLARE(void, attach_thread,
    fluffygc_state* self) {
  apiAssert(CAST(self), !heap_is_attached(CAST(self)), "attempt to double attach");
  heap_attach_thread(CAST(self));
}

FLUFFYGC_DECLARE(void, detach_thread,
    fluffygc_state* self) {
  apiAssert(CAST(self), heap_is_attached(CAST(self)), "attempt to double detach");
  heap_detach_thread(CAST(self));
}

FLUFFYGC_DECLARE(bool, push_frame,
    fluffygc_state* self, int frameSize) {
  checkIfInAttachedThread(CAST(self), __func__);
  
  heap_enter_unsafe_gc(CAST(self));
  bool res = thread_push_frame(getThread(CAST(self)), frameSize);
  heap_exit_unsafe_gc(CAST(self));
  return res;
}

FLUFFYGC_DECLARE(fluffygc_object*, pop_frame,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__);
  
  heap_enter_unsafe_gc(CAST(self));
  struct root_reference* ref = thread_pop_frame(getThread(CAST(self)), CAST(obj));
  heap_exit_unsafe_gc(CAST(self));
  return CAST(ref);
}

FLUFFYGC_DECLARE(fluffygc_object*, new_object,
    fluffygc_state* self, fluffygc_descriptor* desc) {
  checkIfInAttachedThread(CAST(self), __func__); 
  return CAST(heap_obj_new(CAST(self), CAST(desc)));
}

FLUFFYGC_DECLARE(fluffygc_object*, new_local_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  
  heap_enter_unsafe_gc(CAST(self));
  struct root_reference* ref = thread_local_add(getThread(CAST(self)), CAST(obj)->data);
  heap_exit_unsafe_gc(CAST(self));
  return CAST(ref);
}

FLUFFYGC_DECLARE(void, delete_local_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  heap_enter_unsafe_gc(CAST(self));
  thread_local_remove(getThread(CAST(self)), CAST(obj));
  heap_exit_unsafe_gc(CAST(self));
}

FLUFFYGC_DECLARE(fluffygc_object_array*, new_object_array,
    fluffygc_state* self, int size) {
  return (fluffygc_object_array*) heap_array_new(CAST(self), size);
}

static fluffygc_object* readCommonPtr(fluffygc_state* self, fluffygc_object* obj, size_t offset, bool expectArray) { 
  struct root_reference* data = heap_obj_read_ptr(CAST(self), CAST(obj), offset);
  if (data) {
    struct object_info* dataInfo = heap_get_object_info(CAST(self), data->data);
    if (expectArray) 
      apiAssert(CAST(self), dataInfo->type == OBJECT_TYPE_ARRAY, "inconsistent state");
    else  
      apiAssert(CAST(self), dataInfo->type == OBJECT_TYPE_NORMAL, "inconsistent state");
  }
  return CAST(data);
}

static void writeCommonPtr(fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object* data, bool expectArray) { 
  if (data) {
    struct object_info* dataInfo = heap_get_object_info(CAST(self), CAST(data)->data);
    if (expectArray) 
      apiAssert(CAST(self), dataInfo->type == OBJECT_TYPE_ARRAY, "expect array not normal object");
    else  
      apiAssert(CAST(self), dataInfo->type == OBJECT_TYPE_NORMAL, "expect normal object not array");
  }
  heap_obj_write_ptr(CAST(self), CAST(obj), offset, CAST(data));
}

FLUFFYGC_DECLARE(fluffygc_object*, get_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset) {
  checkIfInAttachedThread(CAST(self), __func__); 
  struct object_info* info = heap_get_object_info(CAST(self), CAST(obj)->data);
  apiAssert(CAST(self), info->type == OBJECT_TYPE_NORMAL, "inconsistent state");
  
  struct descriptor* desc = info->typeSpecific.normal.desc;
  int idx = descriptor_get_index_from_offset(desc, offset);
  apiAssert(CAST(self), desc->fields[idx].isArray == false, "field expecting normal object not array");
  return readCommonPtr(self, obj, offset, false);
}

FLUFFYGC_DECLARE(void, set_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object* data) {
  checkIfInAttachedThread(CAST(self), __func__); 
  struct object_info* info = heap_get_object_info(CAST(self), CAST(obj)->data);
  apiAssert(CAST(self), info->type == OBJECT_TYPE_NORMAL, "corrupted?");
  
  struct descriptor* desc = info->typeSpecific.normal.desc;
  int idx = descriptor_get_index_from_offset(desc, offset);
  apiAssert(CAST(self), desc->fields[idx].isArray == false, "field expecting normal object not array");
   
  writeCommonPtr(self, obj, offset, data, false);
}

FLUFFYGC_DECLARE(fluffygc_object_array*, get_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset) {
  checkIfInAttachedThread(CAST(self), __func__); 
  struct object_info* info = heap_get_object_info(CAST(self), CAST(obj)->data);
  apiAssert(CAST(self), info->type == OBJECT_TYPE_NORMAL, "inconsistent state");
  
  struct descriptor* desc = info->typeSpecific.normal.desc;
  int idx = descriptor_get_index_from_offset(desc, offset);
  apiAssert(CAST(self), desc->fields[idx].isArray == false, "field expecting array not normal object");
  return (fluffygc_object_array*) readCommonPtr(self, obj, offset, true);
}

FLUFFYGC_DECLARE(void, set_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object_array* data) {
  checkIfInAttachedThread(CAST(self), __func__); 
  struct object_info* info = heap_get_object_info(CAST(self), CAST(obj)->data);
  apiAssert(CAST(self), info->type == OBJECT_TYPE_NORMAL, "corrupted?");
  
  struct descriptor* desc = info->typeSpecific.normal.desc;
  int idx = descriptor_get_index_from_offset(desc, offset);
  apiAssert(CAST(self), desc->fields[idx].isArray == false, "field expecting array not normal object");
   
  writeCommonPtr(self, obj, offset, (fluffygc_object*) data, true);
}

#endif // IS_ENABLED(CONFIG_API_ENABLE_V1)

