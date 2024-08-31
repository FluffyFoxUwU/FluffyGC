#ifndef UWU_DB9F9B47_DB6A_44E7_9AF4_A70D7B1FFD04_UWU
#define UWU_DB9F9B47_DB6A_44E7_9AF4_A70D7B1FFD04_UWU

#include <stddef.h>

#include <FluffyGC/FluffyGC.h>
#include <FluffyGC/descriptor.h>
#include <FluffyGC/export.h>

typedef struct fluffygc_object fluffygc_object;

FLUFFYGC_EXPORT
void* fluffygc_object_get_data_ptr(fluffygc_object* obj);

FLUFFYGC_EXPORT
fluffygc_object* fluffygc_object_new(fluffygc_state* state, const fluffygc_descriptor* desc, size_t extraSize);

FLUFFYGC_EXPORT
void fluffygc_object_unref(fluffygc_state* state, fluffygc_object* obj);

#endif
