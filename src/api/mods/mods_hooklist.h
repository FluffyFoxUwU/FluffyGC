#ifndef _headers_1691833872_FluffyGC_mods_hooklist
#define _headers_1691833872_FluffyGC_mods_hooklist

#include "FluffyHeap/FluffyHeap.h" // IWYU pragma: keep
#include "attributes.h"
#include "macros.h"
#include "hook/hook.h"

#include "config.h"
#include "FluffyHeap/mods/dma.h"

#include "api/mods/debug/hooks/dma.h" // IWYU pragma: keep
#include "api/mods/debug/hooks/arrays.h" // IWYU pragma: keep

#ifndef ADD_HOOK_TARGET
# define STANDALONE
# define ADD_HOOK_TARGET(target) UNUSED(target)
# define ADD_HOOK_FUNC(target, location, func) hook_register(target, location, func)
#endif

#ifndef ADD_HOOK_FUNC_AND_DECLARE
# define ADD_HOOK_FUNC_AND_DECLARE(target, location, ret, func, ...) \
  ADD_HOOK_FUNC(target, location, func)
#endif

#ifdef STANDALONE
ATTRIBUTE_USED()
void (^uwu)() = ^() {
#endif

//////////////////////////////
// Add new hook target here //
//////////////////////////////

// Example
// ADD_HOOK_TARGET(foo);
// ADD_HOOK_FUNC(foo, );

#if IS_ENABLED(CONFIG_MOD_DMA)

ADD_HOOK_TARGET(fh_object_sync_dma);
  ADD_HOOK_FUNC_AND_DECLARE(fh_object_sync_dma, HOOK_HEAD, void, debug_hook_fh_object_sync_dma_head, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma);
  ADD_HOOK_FUNC_AND_DECLARE(fh_object_sync_dma, HOOK_TAIL, void, debug_hook_fh_object_sync_dma_tail, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma);

ADD_HOOK_TARGET(fh_object_map_dma);
ADD_HOOK_TARGET(fh_object_unmap_dma);

#endif

#ifdef STANDALONE
};
#endif

#endif

