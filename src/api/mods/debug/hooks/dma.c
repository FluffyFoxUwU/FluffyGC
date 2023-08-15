#include "api/pre_code.h"

#include "FluffyHeap.h"
#include "mods/dma.h"

#include "api/mods/debug/hooks/dma.h"

HOOK_FUNCTION_VOID(, debug_hook_fh_object_sync_dma_head, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  
}

HOOK_FUNCTION_VOID(, debug_hook_fh_object_sync_dma_tail, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  
}
