#ifndef _headers_1688801859_FluffyGC_dma_common
#define _headers_1688801859_FluffyGC_dma_common

#include <stdint.h>

#include "FluffyHeap/mods/dma.h"
#include "memory/soc.h"
#include "object/object.h"
#include "util/counter.h"

struct api_mod_state;
struct dma_data {
  uint64_t foreverUniqueID;
  
  struct soc_chunk* chunk;
  fh_dma_ptr apiPtr;
  uint64_t owningObjectID;

  size_t offset;
  size_t size;

  struct {
    struct counter usage;
    struct object_override_info overrideInfo;
    void* allocPtr;
  } dmaCopyingData;
};

// dmaPtr must be already offsetted
struct dma_data* api_mod_dma_common_new_dma_data(struct object* obj, void* dmaPtr, size_t offset, size_t size);
void api_mod_dma_common_free_dma_data(struct dma_data* self, struct object* obj);

int api_mod_dma_check_flags(uint32_t flags);
int api_mod_dma_init(struct api_mod_state* self);
void api_mod_dma_cleanup(struct api_mod_state* self);

#endif

