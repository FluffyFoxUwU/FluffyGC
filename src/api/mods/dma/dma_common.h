#ifndef _headers_1688801859_FluffyGC_dma_common
#define _headers_1688801859_FluffyGC_dma_common

#include <stdint.h>

#include "FluffyHeap/mods/dma.h"
#include "memory/soc.h"
#include "userptr.h"

struct api_mod_state;
struct dma_data {
  uint64_t foreverUniqueID;
  
  struct soc_chunk* chunk;
  fh_dma_ptr apiPtr;
  struct userptr origPtr;
  uint64_t owningObjectID;
};

SOC_DECLARE(api_mod_dma_dma_ptr_cache, struct dma_data);

int api_mod_dma_common_init_dma_data(struct dma_data* self, struct object* obj, void* dmaPtr);
void api_mod_dma_common_cleanup_dma_data(struct dma_data* self, struct object* obj);

int api_mod_dma_check_flags(uint32_t flags);
int api_mod_dma_init(struct api_mod_state* self);
void api_mod_dma_cleanup(struct api_mod_state* self);

#endif

