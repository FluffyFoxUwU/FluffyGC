#ifndef UWU_A21AD3F1_5382_4BDB_ABF2_23BC2B6F416A_UWU
#define UWU_A21AD3F1_5382_4BDB_ABF2_23BC2B6F416A_UWU

#include <stddef.h>
#include <stdint.h>

#define FLUFFYGC_EXPORT \
  [[gnu::visibility("default")]] \
  extern

typedef struct fluffygc_state fluffygc_state;

FLUFFYGC_EXPORT
fluffygc_state* fluffygc_new(size_t heapSize);

FLUFFYGC_EXPORT
void fluffygc_free(fluffygc_state* self);

// Version managements
typedef uint32_t fluffygc_version;

static inline fluffygc_version fluffygc_make_version(uint16_t major, uint8_t minor, uint8_t patch) {
  return (((fluffygc_version) major) << 16) | \
         (((fluffygc_version) minor) <<  8) | \
         ((fluffygc_version)  patch);
}

// API version 1.0.0
#define FLUFFYGC_API_VERSION (0x0001'00'00)

FLUFFYGC_EXPORT
fluffygc_version fluffygc_get_api_version();
FLUFFYGC_EXPORT
fluffygc_version fluffygc_get_impl_version();

FLUFFYGC_EXPORT
const char* fluffygc_get_impl_version_description();
FLUFFYGC_EXPORT
const char* fluffygc_get_impl_short_version_description();

#endif
