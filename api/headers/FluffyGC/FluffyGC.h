#ifndef UWU_A21AD3F1_5382_4BDB_ABF2_23BC2B6F416A_UWU
#define UWU_A21AD3F1_5382_4BDB_ABF2_23BC2B6F416A_UWU

#include <stddef.h>

#include <FluffyGC/version.h>
#include <FluffyGC/export.h>
#include <FluffyGC/gc.h>

typedef struct fluffygc_state fluffygc_state;

FLUFFYGC_EXPORT
fluffygc_state* fluffygc_new(size_t heapSize, const fluffygc_gc* collector);

FLUFFYGC_EXPORT
void fluffygc_free(fluffygc_state* self);

// Version managements
FLUFFYGC_EXPORT
fluffygc_version fluffygc_get_api_version();
FLUFFYGC_EXPORT
fluffygc_version fluffygc_get_impl_version();

FLUFFYGC_EXPORT
const char* fluffygc_get_impl_version_description();
FLUFFYGC_EXPORT
const char* fluffygc_get_impl_short_version_description();

#endif
