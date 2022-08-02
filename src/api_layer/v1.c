#include "config.h"
#include "gc/gc_enums.h"
#include "region.h"

#if IS_ENABLED(CONFIG_API_ENABLE_V1)

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "fluffygc/v1.h"

#include "heap.h"
#include "descriptor.h"
#include "root.h"
#include "thread.h"
#include "util.h"

#define CAST(val) (_Generic((val), \
    struct _7ca2fe74_2f67_4a70_a55d_ba9f6acb6f43*: \
      (struct root_reference*) (val), \
    struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec*: \
      (struct heap*) (val), \
    struct _431d5fb5_4c3f_45e2_9f89_59521ff5cb07*: \
      (struct root_reference*) (val), \
    struct _2aaee2d4_2f50_4685_97f4_63296ae1f585*: \
      (struct descriptor*) (val), \
    struct _3573f4a3_e6d9_4d8f_9176_f5b95a418ecf*: \
      (struct root_reference*) (val), \
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

static void abortWithMessageFormatted(struct heap* heap, const char* fmt, ...) {
  va_list arg;
  va_start(arg, fmt);
  heap_report_vprintf(heap, fmt, arg);
  abort();
  va_end(arg);
}

static void _apiAssert(struct heap* heap, bool cond, const char* func, const char* msg, const char* expr) {
  if (cond)
    return;

  abortWithMessageFormatted(heap, "%s: API assert failed: %s (Expression '%s')", func, msg, expr);
}

#define apiAssert(state, expr, msg) _apiAssert(state, expr, __func__, msg, #expr)

static void _checkType2(struct heap* heap, const char* func, enum object_type gotData, enum object_type expect) {
  assert(gotData != OBJECT_TYPE_UNKNOWN);
  assert(expect != OBJECT_TYPE_UNKNOWN);

  if (gotData != expect) {
    abortWithMessageFormatted(heap, "%s: expect %s got %s", 
          func,
          heap_tostring_object_type(heap, expect),
          heap_tostring_object_type(heap, gotData));
  }
}

static void _checkType(struct heap* heap, const char* func, struct root_reference* data, enum object_type expect) {
  assert(expect != OBJECT_TYPE_UNKNOWN);
  assert(data != NULL);

  struct object_info* info = heap_get_object_info(heap, data->data);
  _checkType2(heap, func, info->type, expect);
}

#define checkType(heap, data, expect) _checkType(heap, __func__, data, expect)
#define checkType2(heap, have, expect) _checkType2(heap, __func__, have, expect)

static fluffygc_object* _readCommonPtr(fluffygc_state* self, const char* func, fluffygc_object* obj, size_t offset, enum object_type expect) { 
  struct object_info* parentInfo = heap_get_object_info(CAST(self), CAST(obj)->data); 
  struct descriptor* desc = parentInfo->typeSpecific.normal.desc;
  if (desc) {
    int idx = descriptor_get_index_from_offset(desc, offset);
    apiAssert(CAST(self), idx >= 0, "Read from non pointer field");
    struct descriptor_field* field = &desc->fields[idx];
    _checkType2(CAST(self), func, field->dataType, expect);
  }
  
  struct root_reference* data = heap_obj_read_ptr(CAST(self), CAST(obj), offset);
  if (data) {
    struct object_info* dataInfo = heap_get_object_info(CAST(self), data->data); 
    apiAssert(CAST(self), dataInfo->type == expect, "inconsistent state?");
  }

  return CAST(data);
}

static void _writeCommonPtr(fluffygc_state* self, const char* func, fluffygc_object* obj, size_t offset, fluffygc_object* data) { 
  struct object_info* parentInfo = heap_get_object_info(CAST(self), CAST(obj)->data); 
  struct descriptor* desc = parentInfo->typeSpecific.normal.desc;
  if (desc) {
    int idx = descriptor_get_index_from_offset(desc, offset);
    apiAssert(CAST(self), idx >= 0, "Write to non pointer field");
    struct descriptor_field* field = &desc->fields[idx];

    _checkType(CAST(self), func, CAST(data), field->dataType);
  }
  heap_obj_write_ptr(CAST(self), CAST(obj), offset, CAST(data));
}

#define writeCommonPtr(self, obj, offset, data) _writeCommonPtr(self, __func__, obj, offset, data);
#define readCommonPtr(self, obj, offset, expect) _readCommonPtr(self, __func__, obj, offset, expect);

FLUFFYGC_DECLARE(fluffygc_state*, new, 
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold,
    int globalRootSize) {
  return CAST(heap_new(youngSize, 
                    oldSize, 
                    metaspaceSize, 
                    localFrameStackSize, 
                    concurrentOldGCthreshold,
                    globalRootSize));
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
    
    apiAssert(CAST(self), arg->fields[i].dataType >= 0 && arg->fields[i].dataType < FLUFFYGC_TYPE_COUNT, "invalid field data type");
    fields[i].dataType = (enum object_type) arg->fields[i].dataType;

    apiAssert(CAST(self), arg->fields[i].type >= 0 && arg->fields[i].type < FLUFFYGC_FIELD_COUNT, "invalid field type");
    fields[i].strength = (enum reference_strength) arg->fields[i].type;
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
  apiAssert(CAST(self), getThread(CAST(self))->topFramePointer > 1, "cannot pop last frame (unbalanced pop and push?)");
  
  struct root_reference* ref = thread_pop_frame(getThread(CAST(self)), CAST(obj));
  heap_exit_unsafe_gc(CAST(self));
  return CAST(ref);
}

FLUFFYGC_DECLARE(fluffygc_object*, new_object,
    fluffygc_state* self, fluffygc_descriptor* desc) {
  checkIfInAttachedThread(CAST(self), __func__); 
  return CAST(heap_obj_new(CAST(self), CAST(desc)));
}

FLUFFYGC_DECLARE(fluffygc_object*, _new_local_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  
  heap_enter_unsafe_gc(CAST(self));
  struct root_reference* ref = NULL;
  if (CAST(obj)->data)
    ref = thread_local_add(getThread(CAST(self)), CAST(obj)->data);
  
  heap_exit_unsafe_gc(CAST(self));
  return CAST(ref);
}

FLUFFYGC_DECLARE(void, _delete_local_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  apiAssert(CAST(self), CAST(obj)->creator == pthread_self() && 
                        CAST(obj)->owner != CAST(self)->globalRoot, "expected current thread's local reference");
  
  heap_enter_unsafe_gc(CAST(self));
  thread_local_remove(getThread(CAST(self)), CAST(obj));
  heap_exit_unsafe_gc(CAST(self));
}

FLUFFYGC_DECLARE(fluffygc_object_array*, new_object_array,
    fluffygc_state* self, int size) {
  return (fluffygc_object_array*) heap_array_new(CAST(self), size);
}

FLUFFYGC_DECLARE(fluffygc_object*, get_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(obj), OBJECT_TYPE_NORMAL);

  return readCommonPtr(self, obj, offset, OBJECT_TYPE_NORMAL);
}

FLUFFYGC_DECLARE(void, set_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object* data) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(obj), OBJECT_TYPE_NORMAL);
  checkType(CAST(self), CAST(data), OBJECT_TYPE_NORMAL);

  writeCommonPtr(self, obj, offset, data);
}

FLUFFYGC_DECLARE(fluffygc_object_array*, get_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(obj), OBJECT_TYPE_NORMAL);
  
  return (fluffygc_object_array*) readCommonPtr(self, obj, offset, OBJECT_TYPE_ARRAY);
}

FLUFFYGC_DECLARE(void, set_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object_array* data) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(obj), OBJECT_TYPE_NORMAL);  
  checkType(CAST(self), CAST(data), OBJECT_TYPE_ARRAY);

  writeCommonPtr(self, obj, offset, (fluffygc_object*) data);
}

FLUFFYGC_DECLARE(int, get_array_length,
    fluffygc_state* self, fluffygc_object_array* array) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(array), OBJECT_TYPE_ARRAY);  
  struct object_info* info = heap_get_object_info(CAST(self), CAST(array)->data); 

  return info->typeSpecific.array.size;
}

FLUFFYGC_DECLARE(fluffygc_object*, get_object_array_element,
    fluffygc_state* self, fluffygc_object_array* array, int index) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(array), OBJECT_TYPE_ARRAY);  
  
  return readCommonPtr(self, (fluffygc_object*) array, sizeof(void*) * index, OBJECT_TYPE_NORMAL);
}

FLUFFYGC_DECLARE(void, set_object_array_element,
    fluffygc_state* self, fluffygc_object_array* array, int index, fluffygc_object* data) {
  checkIfInAttachedThread(CAST(self), __func__); 
  checkType(CAST(self), CAST(array), OBJECT_TYPE_ARRAY);
  checkType(CAST(self), CAST(data), OBJECT_TYPE_NORMAL);  
  
  writeCommonPtr(self, (fluffygc_object*) array, sizeof(void*) * index, data);
}

FLUFFYGC_DECLARE(fluffygc_object*, _new_global_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  
  heap_enter_unsafe_gc(CAST(self));
  pthread_rwlock_wrlock(&CAST(self)->globalRootRWLock);
  apiAssert(CAST(self), CAST(self)->globalRoot->usage >= CAST(self)->globalRoot->size, "Global references table overflowing");

  struct root_reference* ref = NULL;
  if (CAST(obj)->data)
    ref = root_add(CAST(self)->globalRoot, CAST(obj)->data);
  pthread_rwlock_unlock(&CAST(self)->globalRootRWLock);
  heap_exit_unsafe_gc(CAST(self));
  return CAST(ref);
}
FLUFFYGC_DECLARE(void, _delete_global_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  apiAssert(CAST(self), CAST(obj)->owner == CAST(self)->globalRoot, "expected global reference got local reference");

  heap_enter_unsafe_gc(CAST(self));
  pthread_rwlock_wrlock(&CAST(self)->globalRootRWLock);
  root_remove(CAST(self)->globalRoot, CAST(obj));
  pthread_rwlock_unlock(&CAST(self)->globalRootRWLock);
  heap_exit_unsafe_gc(CAST(self));
}

FLUFFYGC_DECLARE(void, trigger_full_gc,
    fluffygc_state* self) {
  checkIfInAttachedThread(CAST(self), __func__); 
  heap_call_gc_blocking(CAST(self), GC_REQUEST_COLLECT_FULL);
}

FLUFFYGC_DECLARE(bool, _is_same_object,
    fluffygc_state* self, fluffygc_object* a, fluffygc_object* b) {
  checkIfInAttachedThread(CAST(self), __func__); 
  heap_enter_unsafe_gc(CAST(self));
  struct region_reference* data = NULL;
  if (b)
    data = CAST(b)->data;

  bool result = CAST(a)->data == data;
  heap_exit_unsafe_gc(CAST(self));
  return result;
}

FLUFFYGC_DECLARE(int, get_current_frame_id,
    fluffygc_state* self) {
  checkIfInAttachedThread(CAST(self), __func__); 
  return getThread(CAST(self))->topFramePointer;
}

FLUFFYGC_DECLARE(fluffygc_weak_object*, _new_weak_global_ref,
    fluffygc_state* self, fluffygc_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  heap_enter_unsafe_gc(CAST(self));
  struct root_reference* ref = CAST(fluffygc_v1_new_global_ref(self, obj));
  if (ref)
    ref->isWeak = true;
  heap_exit_unsafe_gc(CAST(self));
  return (fluffygc_weak_object*) ref;
}

FLUFFYGC_DECLARE(void, delete_weak_global_ref,
    fluffygc_state* self, fluffygc_weak_object* obj) {
  checkIfInAttachedThread(CAST(self), __func__); 
  fluffygc_v1_delete_global_ref(self, (fluffygc_object*) obj);
}

#endif // IS_ENABLED(CONFIG_API_ENABLE_V1)

