#ifndef _headers_1686473038_FluffyGC_dma
#define _headers_1686473038_FluffyGC_dma

#include "../FluffyHeap.h"

#define FH_MOD_DMA_REFERENCES 0x0001
#define FH_MOD_DMA_NONBLOCKING 0x0002

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(void*) fh_object_get_dma(__FLUFFYHEAP_NONNULL(fh_object*) self);
__FLUFFYHEAP_EXPORT void fh_object_put_dma(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(void*) dma);

#endif

