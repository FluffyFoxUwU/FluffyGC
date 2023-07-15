#ifndef _headers_1686473038_FluffyGC_dma
#define _headers_1686473038_FluffyGC_dma

#include <stddef.h>

#include "../FluffyHeap.h"

#define FH_MOD_DMA_NONBLOCKING 0x0001
#define FH_MOD_DMA_ATOMIC      0x0002

#define FH_MOD_DMA_MAP_ATOMIC        0x0001
#define FH_MOD_DMA_MAP_NONBLOCKING   0x0002
#define FH_MOD_DMA_MAP_NO_COPY       0x0004

#define FH_MOD_DMA_ACCESS_READ    0x01
#define FH_MOD_DMA_ACCESS_WRITE   0x02
#define FH_MOD_DMA_ACCESS_RW (FH_MOD_DMA_ACCESS_READ | FH_MOD_DMA_ACCESS_WRITE)

typedef struct fh_fee390b2_8691_4c7c_9fa5_173ca12e57e6 fh_dma_ptr;

struct fh_fee390b2_8691_4c7c_9fa5_173ca12e57e6 {
  __FLUFFYHEAP_NONNULL(void*) ptr;
};

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_dma_ptr*) fh_object_map_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, size_t offset, size_t size, unsigned int mapFlags, unsigned int usage);
__FLUFFYHEAP_EXPORT void fh_object_unmap_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*) dma);
__FLUFFYHEAP_EXPORT void fh_object_sync_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(fh_dma_ptr*) dma);

#endif

