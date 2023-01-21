#ifndef _headers_1673868491_FluffyGC_soc
#define _headers_1673868491_FluffyGC_soc

// Small Object Cache allocator

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bitops.h"
#include "vec.h"
#include "util.h"

// 4 KiB chunk each
#define SOC_CHUNK_SIZE (4 * 1024)
#define SOC_CHUNK_ALIGNMENT SOC_CHUNK_SIZE

#define SOC_MIN_OBJECT_SIZE (sizeof(void*))
#define SOC_MAX_OBJECT_SIZE (sizeof(long) * 8)
#define SOC_MAX_OBJECT_COUNT UTIL_DIV_ROUND_DOWN(SOC_CHUNK_SIZE, SOC_MAX_OBJECT_SIZE)

struct soc_free_node {
  struct soc_free_node* next;
};

struct soc_chunk {
  struct small_object_cache* owner;
  struct soc_free_node* firstFreeNode;
  
  int id;
  bool isReserved;
  int totalObjectsCount;
  
  void* pool;
};

struct small_object_cache {
  size_t objectSize;
  
  vec_t(struct soc_chunk*) chunks;
  unsigned long* partialChunksBitmap;
};

struct small_object_cache* soc_new(size_t objectSize, int reservedChunks);

void* soc_alloc(struct small_object_cache* self);
void soc_dealloc(struct small_object_cache* self, void* ptr);

void soc_free(struct small_object_cache* self);

#endif

