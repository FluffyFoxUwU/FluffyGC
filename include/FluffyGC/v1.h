#ifndef header_1656149251_api_h
#define header_1656149251_api_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "FluffyGC/common.h"

typedef struct _7ca2fe74_2f67_4a70_a55d_ba9f6acb6f43 fluffygc_object;
typedef struct _431d5fb5_4c3f_45e2_9f89_59521ff5cb07 fluffygc_object_array;
typedef struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec fluffygc_state;
typedef struct _2aaee2d4_2f50_4685_97f4_63296ae1f585 fluffygc_descriptor;
typedef struct _3573f4a3_e6d9_4d8f_9176_f5b95a418ecf fluffygc_weak_object;

// Keep this in sync with ./src/descriptor.h
typedef enum _90ad4423_2321_4c8b_bb78_e43d180aa295 {
  FLUFFYGC_FIELD_STRONG = 0,
  FLUFFYGC_FIELD_WEAK = 1,
  FLUFFYGC_FIELD_SOFT = 2,
  
  // Except this
  FLUFFYGC_FIELD_COUNT = 3
} fluffygc_field_type;

// Keep this in sync with ./src/heap.h
typedef enum _4d122aef_0de1_497d_8865_231241cdb3a9 {
  FLUFFYGC_TYPE_UNKNOWN = 0,
  FLUFFYGC_TYPE_NORMAL = 1,
  FLUFFYGC_TYPE_ARRAY = 2,
  
  // Except this
  FLUFFYGC_TYPE_COUNT = 3
} fluffygc_type;

typedef struct _6fe1c279_5c9b_4f5f_8147_fab168b6d75c {
  const char* name;
  size_t offset;

  fluffygc_type dataType;
  fluffygc_field_type type;
} fluffygc_field;

typedef struct b2697c83_e79f_464e_99e3_12bd6c374286 {
  const char* name;

  // Identification of the object
  // owner and type (its recommended
  // that you use pointer to static
  // variable as the value for these)
  //
  // GC never use these except for
  // heap dumps
  uintptr_t ownerID;
  uintptr_t typeID;

  size_t objectSize;
  int fieldCount;
  fluffygc_field* fields;
} fluffygc_descriptor_args;

#define FLUFFYGC_SYM(name) fluffygc_v1_ ## name 

#define FLUFFYGC_DECLARE(ret, name, ...) \
  FLUFFYGC_EXPORT ret FLUFFYGC_SYM(name)(__VA_ARGS__)

#define FLUFFYGC_AS_OBJECT(obj) (_Generic((obj), \
  fluffygc_object*: (obj), \
  fluffygc_object_array*: (fluffygc_object*) obj, \
  fluffygc_weak_object*: (fluffygc_object*) obj))

// State creation and deletion
FLUFFYGC_DECLARE(fluffygc_state*, new, 
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold,
    int globalRootSize);
FLUFFYGC_DECLARE(void, free,
    fluffygc_state* self);

FLUFFYGC_DECLARE(void, trigger_full_gc,
    fluffygc_state* self);

// Descriptor creation and deletion
FLUFFYGC_DECLARE(fluffygc_descriptor*, descriptor_new,
    fluffygc_state* self,
    fluffygc_descriptor_args* arg);
FLUFFYGC_DECLARE(void, descriptor_delete, 
    fluffygc_state* self,
    fluffygc_descriptor* desc);

// Thread attach and detachs
FLUFFYGC_DECLARE(void, attach_thread,
    fluffygc_state* self);
FLUFFYGC_DECLARE(void, detach_thread,
    fluffygc_state* self);

// Frames
//
// -EINVAL is `frameSize` is invalid
// -EOVERFLOW if stack overflowed
// -ENOMEM if memory exhausted
FLUFFYGC_DECLARE(int, push_frame,
    fluffygc_state* self, int frameSize);
FLUFFYGC_DECLARE(fluffygc_object*, pop_frame,
    fluffygc_state* self, fluffygc_object* obj);

// Local reference management
FLUFFYGC_DECLARE(fluffygc_object*, _new_local_ref,
    fluffygc_state* self, fluffygc_object* obj);
FLUFFYGC_DECLARE(void, _delete_local_ref,
    fluffygc_state* self, fluffygc_object* obj);

// Global reference management
FLUFFYGC_DECLARE(fluffygc_object*, _new_global_ref,
    fluffygc_state* self, fluffygc_object* obj);
FLUFFYGC_DECLARE(void, _delete_global_ref,
    fluffygc_state* self, fluffygc_object* obj);

// Object creation
FLUFFYGC_DECLARE(fluffygc_object*, new_object,
    fluffygc_state* self, fluffygc_descriptor* desc);
FLUFFYGC_DECLARE(fluffygc_object_array*, new_object_array,
    fluffygc_state* self, int size);

// Object operations
FLUFFYGC_DECLARE(fluffygc_object*, get_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset);
FLUFFYGC_DECLARE(void, set_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object* data);
FLUFFYGC_DECLARE(fluffygc_object_array*, get_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset);
FLUFFYGC_DECLARE(void, set_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object_array* data);

FLUFFYGC_DECLARE(void, obj_read_data,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, size_t size, void* result);
FLUFFYGC_DECLARE(void, obj_write_data,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, size_t size, const void* data);

// Array operations
FLUFFYGC_DECLARE(int, get_array_length,
    fluffygc_state* self, fluffygc_object_array* array);
FLUFFYGC_DECLARE(fluffygc_object*, get_object_array_element,
    fluffygc_state* self, fluffygc_object_array* array, int index);
FLUFFYGC_DECLARE(void, set_object_array_element,
    fluffygc_state* self, fluffygc_object_array* array, int index, fluffygc_object* data);

// Comparisons
FLUFFYGC_DECLARE(bool, _is_same_object,
    fluffygc_state* self, fluffygc_object* a, fluffygc_object* b);

// Weak globals
FLUFFYGC_DECLARE(fluffygc_weak_object*, _new_weak_global_ref,
    fluffygc_state* self, fluffygc_object* obj);
FLUFFYGC_DECLARE(void, delete_weak_global_ref,
    fluffygc_state* self, fluffygc_weak_object* obj);

// Misc
FLUFFYGC_DECLARE(int, get_current_frame_id,
    fluffygc_state* self);

// Get object critical 
// You shall not modify the pointer fields
FLUFFYGC_DECLARE(void*, get_object_critical, fluffygc_state* self, fluffygc_object* obj, bool* isCopy);
FLUFFYGC_DECLARE(void, release_object_critical, fluffygc_state* self, fluffygc_object* obj, void* ptr);

// Macro wrappers for type safety
#define fluffygc_v1_new_local_ref(_, obj) ((typeof(obj)) fluffygc_v1__new_local_ref((_), FLUFFYGC_AS_OBJECT(obj)))
#define fluffygc_v1_delete_local_ref(_, obj) fluffygc_v1__delete_local_ref((_), FLUFFYGC_AS_OBJECT(obj))
#define fluffygc_v1_new_global_ref(_, obj) ((typeof(obj)) fluffygc_v1__new_global_ref((_), FLUFFYGC_AS_OBJECT(obj)))
#define fluffygc_v1_delete_global_ref(_, obj) fluffygc_v1__delete_global_ref((_), FLUFFYGC_AS_OBJECT(obj))
#define fluffygc_v1_is_same_object(_, objA, objB) fluffygc_v1__is_same_object((_), FLUFFYGC_AS_OBJECT(objA), FLUFFYGC_AS_OBJECT(objB))
#define fluffygc_v1_new_weak_global_ref(_, obj) fluffygc_v1__new_weak_global_ref((_), FLUFFYGC_AS_OBJECT(obj))

// Convenience wrapper
#define fluffygc_read_data(heap, result, obj, structure, member) do { \
  static_assert(_Generic((*result), \
    typeof(typeof(((structure*) 0)->member)): true,\
    default: false\
  ), "Destination type is not the same as source type"); \
  fluffygc_v1_obj_read_data((heap), (obj), offsetof(structure, member), sizeof(((structure*) 0)->member), (result)); \
} while(0)

#define fluffygc_write_data(heap, obj, structure, member, data) do { \
  typeof(((structure*) 0)->member) tmp = (data); \
  fluffygc_v1_obj_write_data((heap), (obj), offsetof(structure, member), sizeof(tmp), &tmp); \
} while (0)

#endif


