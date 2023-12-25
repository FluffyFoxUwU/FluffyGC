#ifndef _headers_1689407839_FluffyGC_mods
#define _headers_1689407839_FluffyGC_mods

#include <stdint.h>

#include "FluffyHeap/FluffyHeap.h"
#include "managed_heap.h"
#include "debug/debug.h"

struct api_mod_state;
struct api_mod_info {
  bool available;
  uint32_t supportedFlags;
  int (*checkFlags)(uint32_t flags);
  
  // Heap is available at these functions call
  int (*init)(struct api_mod_state* state);
  void (*cleanup)(struct api_mod_state* state);
};

struct api_mod_state {
  bool enabled;
  struct api_mod_info* info;
  uint32_t flags;
  
  union {
    struct debug_mod_state debugMod;
  } data;
};

int api_mods_init(struct managed_heap* heap);
void api_mods_cleanup(struct managed_heap* heap);

// Returns NULL on unknown
const char* api_mods_tostring(fh_mod mod);

#endif

