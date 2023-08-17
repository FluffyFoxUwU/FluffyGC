#include <inttypes.h>

#include "dma_common.h"
#include "logger/logger.h"
#include "object/object.h"
#include "managed_heap.h"

SOC_DEFINE(api_mod_dma_dma_ptr_cache, SOC_DEFAULT_CHUNK_SIZE, struct dma_data);

int api_mod_dma_common_init_dma_data(struct dma_data* self, struct object* obj, void* dmaPtr) {
  self->origPtr = USERPTR(dmaPtr);
  self->apiPtr.ptr = (void*) dmaPtr;
  self->owningObjectID = obj->movePreserve.foreverUniqueID;
  
  self->foreverUniqueID = managed_heap_generate_dma_ptr_id();
  return 0;
}

void api_mod_dma_common_cleanup_dma_data(struct dma_data* self, struct object* obj) {
  if (obj->movePreserve.foreverUniqueID != self->owningObjectID)
    pr_crit("Trying unmapping DMA pointer %p (associated with %" PRIu64") using %" PRIu64, &self->apiPtr, self->owningObjectID, obj->movePreserve.foreverUniqueID);
}
