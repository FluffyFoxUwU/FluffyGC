#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stddef.h>

#include "FluffyHeap/FluffyHeap.h"
#include "FluffyHeap/mods/dma.h"

#include "api/api.h"
#include "api/mods/debug/common.h"
#include "context.h"
#include "macros.h"
#include "logger/logger.h"
#include "dma_common.h"
#include "object/object.h"
#include "util/counter.h"
#include "util/util.h"

DEFINE_LOGGER_STATIC(logger, "DMA Mod (Copying Flavour)");

#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&logger)

API_FUNCTION_DEFINE(__FLUFFYHEAP_NULLABLE(fh_dma_ptr*), fh_object_map_dma, __FLUFFYHEAP_NONNULL(fh_object*), self, size_t, offset, size_t, size, unsigned long, mapFlags, unsigned long, usage) {
  UNUSED(mapFlags);
  UNUSED(usage);

  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));
  object_block_ptr(obj);
  
  // A DMA already present use that instead
  struct object_override_info* overrideInfo;
  if ((overrideInfo = object_get_override_ptr(obj))) {
    struct dma_data* dmaData = container_of(overrideInfo, struct dma_data, dmaCopyingData.overrideInfo);
    uint64_t oldUsage = counter_increment(&dmaData->dmaCopyingData.usage) + 1;
    object_unblock_ptr(obj);
    context_unblock_gc();
    
    pr_debug("Reused DMA ptr#%ld, usage incremented to %" PRIu64, dmaData->foreverUniqueID, oldUsage + 1);
    return &dmaData->apiPtr;
  }

  // Get fixed area for DMA
  void* dmaPtr = (char*) util_aligned_alloc(alignof(max_align_t), obj->objectSize);
  if (!dmaPtr)
    goto failure_alloc_dma_ptr;

  // Create DMA data
  struct dma_data* dma = api_mod_dma_common_new_dma_data(obj, (char*) dmaPtr + offset, offset, size);
  if (!dma)
    goto failure_new_dma_data;
  dma->dmaCopyingData.allocPtr = dmaPtr;
  dma->dmaCopyingData.overrideInfo.overridePtr = dmaPtr;

  // Here the copying part
  memcpy(dmaPtr, object_get_backing_ptr(obj), obj->objectSize);
  
  // Any read or write will go through the new dmaPtr
  object_set_override_ptr(obj, &dma->dmaCopyingData.overrideInfo);
  object_unblock_ptr(obj);
  context_unblock_gc();

  pr_debug("Newly mapped at %p for object %s (DMA ptr#%ld offsetted by %zu)", dma->apiPtr.ptr, debug_get_unique_name(self), dma->foreverUniqueID, offset);
  return &dma->apiPtr;

failure_new_dma_data:
  free(dmaPtr);
failure_alloc_dma_ptr:
  object_unblock_ptr(obj);
  context_unblock_gc();
  return NULL;
}

API_FUNCTION_DEFINE_VOID(fh_object_unmap_dma, __FLUFFYHEAP_NONNULL(fh_object*), self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*), dma) {
  struct dma_data* dmaData = container_of(dma, struct dma_data, apiPtr);
  uint64_t oldUsage;
  if ((oldUsage = counter_decrement(&dmaData->dmaCopyingData.usage)) > 0) {
    pr_debug("Decrement DMA ptr#%ld usage to %" PRIu64, dmaData->foreverUniqueID, oldUsage - 1);
    return;
  }

  context_block_gc();
  struct object* obj = root_ref_get(INTERN(self));

  // Last user of current DMA do write back
  // and do clean up
  if (dmaData->dmaCopyingData.allocPtr != NULL) {
    object_block_ptr(obj);
    memcpy(object_get_backing_ptr(obj), dmaData->dmaCopyingData.allocPtr, obj->objectSize);
    free(dmaData->dmaCopyingData.allocPtr);
    object_set_override_ptr(obj, NULL);
    object_unblock_ptr(obj);
  }

  pr_debug("Umapped %p for object %s (DMA ptr#%ld)", dmaData->apiPtr.ptr, debug_get_unique_name(self), dmaData->foreverUniqueID);
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


