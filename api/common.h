#ifndef UWU_FA033D7E_E7E3_4C9B_9416_7F5E53507E54_UWU
#define UWU_FA033D7E_E7E3_4C9B_9416_7F5E53507E54_UWU

// These used by the macros
#include "api/headers/FluffyGC.h" // IWYU pragma: keep
#include "heap/heap.h" // IWYU pragma: keep

#define API_INTERN(x) _Generic((x), \
  fluffygc_state*: (struct heap*) (x) \
)

#define API_EXTERN(x) _Generic((x), \
  struct heap*: (fluffygc_state*) (x) \
)

#endif
