#ifndef _headers_1691834364_FluffyGC_dma
#define _headers_1691834364_FluffyGC_dma

#include "FluffyHeap.h"
#include "hook/hook.h"
#include "mods/dma.h"

HOOK_FUNCTION_DECLARE(, void, debug_hook_fh_object_sync_dma_head, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma);
HOOK_FUNCTION_DECLARE(, void, debug_hook_fh_object_sync_dma_tail, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma);

#endif

