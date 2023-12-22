#include "FluffyHeap.h"
#include "mods/dma.h"

#include "api/mods/debug/hooks/dma.h"
#include "macros.h"

// TODO: Add checking for DMA
HOOK_FUNCTION_VOID(, debug_hook_fh_object_sync_dma_head, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  UNUSED(dma);
  UNUSED(self);
}

HOOK_FUNCTION_VOID(, debug_hook_fh_object_sync_dma_tail, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  UNUSED(self);
  UNUSED(dma);
}


