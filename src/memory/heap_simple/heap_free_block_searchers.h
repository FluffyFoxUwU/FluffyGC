#ifndef _headers_1674461586_FluffyGC_heap_free_block_searchers
#define _headers_1674461586_FluffyGC_heap_free_block_searchers

#include <stddef.h>

struct heap_block;
struct heap;

struct heap_block* heap_find_free_block(struct heap* self, size_t blockSize);

#endif

