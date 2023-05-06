#ifndef _headers_1673868491_FluffyGC_soc
#define _headers_1673868491_FluffyGC_soc

// Small Object Cache allocator
// Can work on non power of two block sizes
// but it would warn about it

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bitops.h"
#include "vec.h"
#include "util/util.h"

// 4 KiB chunk each
#define SOC_DEFAULT_CHUNK_SIZE (4 * 1024)

struct soc_free_node {
  struct soc_free_node* next;
};

#define SOC_MIN_OBJECT_SIZE (sizeof(struct soc_free_node))

// Lowest number of object can fit into a chunk
// before losing too much overhead (number is arbitrary)
#define SOC_MIN_OBJECT_COUNT (8)

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
  size_t alignment;
  size_t chunkSize;
  
  vec_t(struct soc_chunk*) chunks;
  unsigned long* partialChunksBitmap;
};

struct small_object_cache* soc_new_with_chunk_size(size_t chunkSize, size_t alignment, size_t objectSize, int preallocCount);
struct small_object_cache* soc_new(size_t alignment, size_t objectSize, int preallocCount);

void* soc_alloc(struct small_object_cache* self);
void soc_dealloc(struct small_object_cache* self, void* ptr);

// Only used in btree to avoid dead infinite loops
// because recursive relying on each other
void* soc_alloc_explicit(struct small_object_cache* self, struct soc_chunk** chunk);
void soc_dealloc_explicit(struct small_object_cache* self, struct soc_chunk* chunk, void* ptr);

void soc_free(struct small_object_cache* self);

#endif

