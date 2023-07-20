#include <string.h>
#include <threads.h>

#include "api/pre_code.h"

#include "api/mods/debug/debug.h"
#include "api/mods/dma_common.h"

#include "FluffyHeap.h"
#include "mods.h"
#include "config.h"
#include "panic.h"
#include "util/util.h"

static struct api_mod_info mods[FH_MOD_COUNT] = {
# if IS_ENABLED(CONFIG_MOD_DMA)
  [FH_MOD_DMA] = {
    .available = true,
    .supportedFlags = FH_MOD_DMA_NONBLOCKING | FH_MOD_DMA_ATOMIC,
    .checkFlags = api_mod_dma_check_flags,
    
    .init = api_mod_dma_init,
    .cleanup = api_mod_dma_cleanup
  },
# endif

# if IS_ENABLED(CONFIG_MOD_DEBUG)
  [FH_MOD_DEBUG] = {
    .available = true,
    .supportedFlags = 0,
    .checkFlags = api_mod_debug_check_flags,
    
    .init = api_mod_debug_init,
    .cleanup = api_mod_debug_cleanup
  },
# endif
};

static thread_local struct api_mod_state modStates[FH_MOD_COUNT] = {};
int api_mods_init(struct managed_heap* heap) {
  memcpy(heap->api.state->modStates, modStates, sizeof(modStates));
  for (int i = 0; i < FH_MOD_COUNT; i++) {
    if (!heap->api.state->modStates[i].enabled)
      continue;
    
    int ret = heap->api.state->modStates[i].info->init(&heap->api.state->modStates[i]);
    if (ret < 0)
      panic("Not implemented yet!");
  }
  return 0;
}

void api_mods_cleanup(struct managed_heap* heap) {
}

static struct api_mod_info* getMod(enum fh_mod mod) {
  if (mod <= 0 || mod >= ARRAY_SIZE(mods))
    return NULL;
  return mods[mod].available ? &mods[mod] : NULL;
}

__FLUFFYHEAP_EXPORT uint32_t fh_get_flags(enum fh_mod mod) {
  if (!getMod(mod))
    return 0;
  return modStates[mod].flags | FH_MOD_WAS_ENABLED;
}

__FLUFFYHEAP_EXPORT bool fh_check_mod(enum fh_mod mod, uint32_t flags) {
  struct api_mod_info* info = getMod(mod);
  if (!info)
    return false;
  
  if ((info->supportedFlags & flags) != flags)
    return false;
  return info->checkFlags(flags);
}

__FLUFFYHEAP_EXPORT int fh_enable_mod(enum fh_mod mod, uint32_t flags) {
  struct api_mod_info* info;
  if (!(info = getMod(mod)))
    return -ENODEV;
  
  if (modStates[mod].enabled == true)
    return -EBUSY;
  
  if ((info->supportedFlags & flags) != flags)
    return -EINVAL;
  if (info->checkFlags(flags) < 0)
    return -EINVAL;
  
  modStates[mod].enabled = true;
  modStates[mod].flags = flags;
  modStates[mod].info = info;
  return 0;
}

__FLUFFYHEAP_EXPORT void fh_disable_mod(enum fh_mod mod) {
  if (!getMod(mod))
    return;
  modStates[mod].enabled = false;
}

