#include <stddef.h>
#include <inttypes.h>
#include <stddef.h>

#include "dma_common.h"
#include "logger/logger.h"
#include "memory/soc.h"
#include "object/object.h"
#include "managed_heap.h"

SOC_DEFINE_STATIC(dmaDataCache, SOC_DEFAULT_CHUNK_SIZE, struct dma_data);

struct dma_data* api_mod_dma_common_new_dma_data(struct object* obj, void* dmaPtr, size_t offset, size_t size) {
  struct soc_chunk* chunk;
  struct dma_data* self = soc_alloc_explicit(dmaDataCache, &chunk);
  if (!self)
    return NULL;

  *self = (struct dma_data) {
    .apiPtr.ptr = dmaPtr,
    .owningObjectID = obj->movePreserve.foreverUniqueID,
    .offset = offset,
    .foreverUniqueID = managed_heap_generate_dma_ptr_id(),
    .chunk = chunk,
    .size = size
  };

  return self;
}

void api_mod_dma_common_free_dma_data(struct dma_data* self, struct object* obj) {
  if (!self)
    return;

  if (obj->movePreserve.foreverUniqueID != self->owningObjectID)
    pr_crit("Trying unmapping DMA#%" PRIu64 " pointer %p (associated with %" PRIu64" object) using %" PRIu64 " object", self->foreverUniqueID, &self->apiPtr, self->owningObjectID, obj->movePreserve.foreverUniqueID);
  soc_dealloc_explicit(dmaDataCache, self->chunk, self);
}


