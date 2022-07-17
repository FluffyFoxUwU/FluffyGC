#ifndef header_1656149251_api_h
#define header_1656149251_api_h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FLUFFYGC_EXPORT

typedef struct _7ca2fe74_2f67_4a70_a55d_ba9f6acb6f43 fluffygc_object;
typedef struct _c2e9d5e4_e9c3_492a_a874_6c5c5ca70dec fluffygc_state;
typedef struct _2aaee2d4_2f50_4685_97f4_63296ae1f585 fluffygc_descriptor;

typedef struct _6fe1c279_5c9b_4f5f_8147_fab168b6d75c {
  const char* name;
  size_t offset;
} fluffygc_field;
typedef struct _bf9d30d9_7511_45c6_8091_3c2c86096df0 {
  const char* name;
  
  uintptr_t ownerID;
  uintptr_t typeID;
} fluffygc_typeid;

typedef struct {
  
}* e58aec1c_b629_4e1f_8754_ec0e8e6f7158;

/*
FLUFFYGC_EXPORT fluffygc_state* fluffygc_new_v1(
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold);
FLUFFYGC_EXPORT void fluffygc_free(
    fluffygc_state* self);

FLUFFYGC_EXPORT fluffygc_descriptor* fluffygc_descriptor_new_v1(
    fluffygc_state* self,
    fluffygc_typeid typeID,
    size_t objectSize,
    int numFields,
    fluffygc_field* fields);
FLUFFYGC_EXPORT void fluffygc_descriptor_release(
    fluffygc_state* self, 
    fluffygc_descriptor* desc);

FLUFFYGC_EXPORT bool fluffygc_attach_thread(
    fluffygc_state* self);
FLUFFYGC_EXPORT bool fluffygc_deattach_thread(
    fluffygc_state* self);

FLUFFYGC_EXPORT bool fluffygc_push_local_frame(
    fluffygc_state* self);
FLUFFYGC_EXPORT bool fluffygc_pop_local_frame(
    fluffygc_state* self);
*/

#endif


