#ifndef UWU_F7F1664F_2BBF_4C6F_9546_0F9B526BEB40_UWU
#define UWU_F7F1664F_2BBF_4C6F_9546_0F9B526BEB40_UWU

#include <stddef.h>

#include <FluffyGC/FluffyGC.h>

// A structure describing about GC
typedef struct fluffygc_gc {
  fluffygc_version version;
  const char* name;
  const char* id;
  const char* description;
} fluffygc_gc;

FLUFFYGC_EXPORT
size_t fluffygc_gc_get_algorithmns(struct fluffygc_gc* algorithmns, size_t maxCount);

#endif
