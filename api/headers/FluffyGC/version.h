#ifndef UWU_E2355FBD_CEE0_4E0F_8006_6B4A621EDAB1_UWU
#define UWU_E2355FBD_CEE0_4E0F_8006_6B4A621EDAB1_UWU

#include <stdint.h>

typedef uint32_t fluffygc_version;

static inline fluffygc_version fluffygc_make_version(uint16_t major, uint8_t minor, uint8_t patch) {
  return (((fluffygc_version) major) << 16) | \
         (((fluffygc_version) minor) <<  8) | \
         ((fluffygc_version)  patch);
}

// API version 1.0.0
#define FLUFFYGC_API_VERSION (0x0001'00'00)

#endif
