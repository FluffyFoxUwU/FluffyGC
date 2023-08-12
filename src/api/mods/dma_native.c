#include <stdatomic.h>

#include "api/pre_code.h"

#include "FluffyHeap.h"
#include "context.h"
#include "logger/logger.h"
#include "memory/soc.h"
#include "mods/dma.h"

#include "object/object.h"
#include "util/util.h"
#include "dma_common.h"

DEFINE_LOGGER_STATIC(logger, "DMA Mod (Native)");

#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&logger)

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_dma_ptr*) fh_object_map_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, size_t offset, size_t size, unsigned int mapFlags, unsigned int usage) {
  struct soc_chunk* chunk;
  struct dma_data* dma = soc_alloc_explicit(api_mod_dma_dma_ptr_cache, &chunk);
  if (!dma)
    return NULL;
  dma->chunk = chunk;
  
  context_block_gc();
  struct object* obj = atomic_load(&INTERN(self)->obj);
  dma->origPtr = obj->dataPtr;
  dma->apiPtr.ptr = (void*) obj->dataPtr.ptr + offset;
  context_unblock_gc();
  return &dma->apiPtr;
}

__FLUFFYHEAP_EXPORT void fh_object_unmap_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*) dma) {
  struct dma_data* dmaPtr = container_of(dma, struct dma_data, apiPtr);
  soc_dealloc_explicit(api_mod_dma_dma_ptr_cache, dmaPtr->chunk, dmaPtr);
}

__FLUFFYHEAP_EXPORT void fh_object_sync_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*) dma) {
  atomic_thread_fence(memory_order_seq_cst);
}

int api_mod_dma_check_flags(uint32_t flags) {
  return 0;
}

int api_mod_dma_init(struct api_mod_state* self) {
  pr_info("Initialized");
  return 0;
}

void api_mod_dma_cleanup(struct api_mod_state* self) {
  pr_info("Done cleaning up");
  return;
}
