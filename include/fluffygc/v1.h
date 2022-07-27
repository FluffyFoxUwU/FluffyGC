#ifndef header_1656149251_api_h
#define header_1656149251_api_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "fluffygc/common.h"

typedef struct _7ca2fe74_2f67_4a70_a55d_ba9f6acb6f43 fluffygc_object;
typedef struct _431d5fb5_4c3f_45e2_9f89_59521ff5cb07 fluffygc_object_array;
typedef struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec fluffygc_state;
typedef struct _2aaee2d4_2f50_4685_97f4_63296ae1f585 fluffygc_descriptor;

// Keep this in sync with ./src/descriptor.h
typedef enum _90ad4423_2321_4c8b_bb78_e43d180aa295 {
  FLUFFYGC_FIELD_UNKNOWN = 0,
  FLUFFYGC_FIELD_STRONG,
  FLUFFYGC_FIELD_WEAK
} fluffygc_field_type;

typedef struct _6fe1c279_5c9b_4f5f_8147_fab168b6d75c {
  const char* name;
  size_t offset;
  bool isArray;

  fluffygc_field_type type;
} fluffygc_field;

typedef struct b2697c83_e79f_464e_99e3_12bd6c374286 {
  const char* name;
  
  uintptr_t ownerID;
  uintptr_t typeID;

  size_t objectSize;
  int fieldCount;
  fluffygc_field* fields;
} fluffygc_descriptor_args;

#define FLUFFYGC_SYM(name) fluffygc_v1_ ## name 

#define FLUFFYGC_DECLARE(ret, name, ...) \
  FLUFFYGC_EXPORT ret FLUFFYGC_SYM(name)(__VA_ARGS__)

// State creation and deletion
FLUFFYGC_DECLARE(fluffygc_state*, new, 
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold);
FLUFFYGC_DECLARE(void, free,
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
FLUFFYGC_DECLARE(bool, push_frame,
    fluffygc_state* self, int frameSize);
FLUFFYGC_DECLARE(fluffygc_object*, pop_frame,
    fluffygc_state* self, fluffygc_object* obj);

// Reference management
FLUFFYGC_DECLARE(fluffygc_object*, new_local_ref,
    fluffygc_state* self, fluffygc_object* obj);
FLUFFYGC_DECLARE(void, delete_local_ref,
    fluffygc_state* self, fluffygc_object* obj);

// Object creation
FLUFFYGC_DECLARE(fluffygc_object*, new_object,
    fluffygc_state* self, fluffygc_descriptor* desc);
FLUFFYGC_DECLARE(fluffygc_object_array*, new_object_array,
    fluffygc_state* self, int size);

// Objects
FLUFFYGC_DECLARE(fluffygc_object*, get_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset);
FLUFFYGC_DECLARE(void, set_object_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object* data);

// Arrays
FLUFFYGC_DECLARE(fluffygc_object_array*, get_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset);
FLUFFYGC_DECLARE(void, set_array_field,
    fluffygc_state* self, fluffygc_object* obj, size_t offset, fluffygc_object_array* data);

#endif


