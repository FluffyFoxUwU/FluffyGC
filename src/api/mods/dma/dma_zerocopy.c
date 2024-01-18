#include <stddef.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>

#include "api/api.h"

#include "FluffyHeap/FluffyHeap.h"
#include "FluffyHeap/mods/dma.h"

#include "context.h"
#include "logger/logger.h"

#include "object/object.h"
#include "util/util.h"
#include "dma_common.h"
#include "macros.h"

#include "api/mods/debug/common.h"

DEFINE_LOGGER_STATIC(logger, "DMA Mod (Zero Copy Flavour)");

#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&logger)

// Zero Copy DMA is fastest of all by directly providing
// backing pointer without copy

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_dma_ptr*), fh_object_map_dma, __FLUFFYHEAP_NONNULL(fh_object*), self, size_t, offset, size_t, size, unsigned long, mapFlags, unsigned long, usage) {
  UNUSED(mapFlags);
  UNUSED(usage);

  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  object_ptr_use_start(obj);
  struct dma_data* dma = api_mod_dma_common_new_dma_data(obj, (char*) object_get_ptr(obj) + offset, offset, size);
  object_ptr_use_end(obj);
  context_unblock_gc();
  
  if (!dma)
    return NULL;
  
  pr_debug("Mapped at %p for object %s (DMA ptr#%" PRIu64 " offsetted by %zu)", dma->apiPtr.ptr, debug_get_unique_name(self), dma->foreverUniqueID, offset);
  return &dma->apiPtr;
}

API_FUNCTION_DEFINE_VOID(fh_object_unmap_dma, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  struct dma_data* dmaData = container_of(dma, struct dma_data, apiPtr);
  
  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  pr_debug("Umapped %p for object %s (DMA ptr#%" PRIu64 ")", dmaData->apiPtr.ptr, debug_get_unique_name(self), dmaData->foreverUniqueID);
  api_mod_dma_common_free_dma_data(dmaData, obj);
  context_unblock_gc();
}

API_FUNCTION_DEFINE_VOID(fh_object_sync_dma, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  UNUSED(self);
  UNUSED(dma);
  atomic_thread_fence(memory_order_seq_cst);
}

int api_mod_dma_check_flags(uint32_t flags) {
  UNUSED(flags);
  return 0;
}

int api_mod_dma_init(struct api_mod_state* self) {
  UNUSED(self);
  return 0;
}

void api_mod_dma_cleanup(struct api_mod_state* self) {
  UNUSED(self);
  return;
}


