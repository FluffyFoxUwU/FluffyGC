#ifndef _headers_1688801859_FluffyGC_dma_common
#define _headers_1688801859_FluffyGC_dma_common

#include "mods/dma.h"
#include "object/object.h"
#include "memory/soc.h"

struct api_mod_state;
struct dma_data {
  struct soc_chunk* chunk;
  fh_dma_ptr apiPtr;
  struct userptr origPtr;
};

SOC_DECLARE(api_mod_dma_dma_ptr_cache, struct dma_data);

int api_mod_dma_check_flags(uint32_t flags);
int api_mod_dma_init(struct api_mod_state* self);
void api_mod_dma_cleanup(struct api_mod_state* self);

#endif

