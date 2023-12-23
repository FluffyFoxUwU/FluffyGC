#include <string.h>
#include <threads.h>

#include "api/mods/debug/debug.h"
#include "api/mods/dma_common.h"

#include "mods/dma.h"
#include "mods/debug.h"

#include "FluffyHeap.h"
#include "mods.h"
#include "config.h"
#include "util/util.h"

static struct api_mod_info mods[FH_MOD_COUNT] = {
# if IS_ENABLED(CONFIG_MOD_DMA)
  [FH_MOD_DMA] = {
    .available = true,
    .supportedFlags = 0,
    .checkFlags = api_mod_dma_check_flags,
    
    .init = api_mod_dma_init,
    .cleanup = api_mod_dma_cleanup
  },
# endif

# if IS_ENABLED(CONFIG_MOD_DEBUG)
  [FH_MOD_DEBUG] = {
    .available = true,
    .supportedFlags = FH_MOD_DEBUG_DONT_KEEP_GOING,
    .checkFlags = debug_check_flags,
    
    .init = debug_init,
    .cleanup = debug_cleanup
  },
# endif
};

static thread_local struct api_mod_state currentModStates[FH_MOD_COUNT] = {};
int api_mods_init(struct managed_heap* heap) {
  struct api_mod_state* modStates = heap->api.state->modManager.modStates;
  memcpy(modStates, currentModStates, sizeof(currentModStates));
  int modInitCounts = 0;
  int ret = 0;
  
  for (; modInitCounts < FH_MOD_COUNT; modInitCounts++) {
    if (!modStates[modInitCounts].enabled)
      continue;
    
    if ((ret = modStates[modInitCounts].info->init(&modStates[modInitCounts])) < 0)
      break;
  }
  
  heap->api.state->modManager.modIteratedCountDuringInit = modInitCounts;
  return ret;
}

void api_mods_cleanup(struct managed_heap* heap) {
  struct api_mod_state* modStates = heap->api.state->modManager.modStates;
  int modInitCounts = 0;
  for (; modInitCounts > 0; modInitCounts--) {
    int current = modInitCounts - 1;
    if (!modStates[current].enabled)
      continue;
    
    modStates[current].info->cleanup(&modStates[current]);
  }
  heap->api.state->modManager.modIteratedCountDuringInit = 0;
}

static struct api_mod_info* getMod(fh_mod mod) {
  if (mod <= 0 || mod >= ARRAY_SIZE(mods))
    return NULL;
  return mods[mod].available ? &mods[mod] : NULL;
}

API_FUNCTION_DEFINE(unsigned long, fh_get_flags, fh_mod, mod) {
  if (!getMod(mod))
    return 0;
  return currentModStates[mod].flags | FH_MOD_WAS_ENABLED;
}

API_FUNCTION_DEFINE(bool, fh_check_mod, fh_mod, mod, unsigned long, flags) {
  struct api_mod_info* info = getMod(mod);
  if (!info)
    return false;
  
  if ((info->supportedFlags & flags) != flags)
    return false;
  return info->checkFlags(flags);
}

API_FUNCTION_DEFINE(int, fh_enable_mod, fh_mod, mod, unsigned long, flags) {
  struct api_mod_info* info;
  if (!(info = getMod(mod)))
    return -ENODEV;
  
  if ((info->supportedFlags & flags) != flags)
    return -EINVAL;
  if (info->checkFlags(flags) < 0)
    return -EINVAL;
  if (currentModStates[mod].enabled == true) {
    // Flags given is not subset of already enabled one
    if ((currentModStates[mod].flags & flags) != flags)
      return -EBUSY;
    
    // Add the flags
    flags |= currentModStates[mod].flags;
  }
  
  currentModStates[mod].enabled = true;
  currentModStates[mod].flags = flags;
  currentModStates[mod].info = info;
  return 0;
}

API_FUNCTION_DEFINE_VOID(fh_disable_mod, fh_mod, mod) {
  if (!getMod(mod))
    return;
  currentModStates[mod].enabled = false;
}

const char* api_mods_tostring(fh_mod mod) {
  switch (mod) {
    case FH_MOD_DMA:
      return "FH_MOD_DMA";
    case FH_MOD_DEBUG:
      return "FH_MOD_DEBUG";
    case FH_MOD_COUNT:
    case FH_MOD_UNKNOWN:
      break;
  }
  return NULL;
}
