#ifndef _headers_1673868491_FluffyGC_soc
#define _headers_1673868491_FluffyGC_soc

// Small Object Cache allocator
// Can work on non power of two block sizes
// but it would warn about it

#include <stdbool.h>

#include "util/list_head.h"
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
  struct list_head list;
  struct small_object_cache* owner;
  struct soc_free_node* firstFreeNode;
  
  bool isReserved;
  
  void* pool;
};

struct small_object_cache {
  bool isStatic;
  size_t objectSize;
  size_t alignment;
  size_t chunkSize;
  
  struct list_head fullChunksList;
  struct list_head partialChunksList;
};

struct small_object_cache* soc_new_with_chunk_size(size_t chunkSize, size_t alignment, size_t objectSize, int preallocCount);
struct small_object_cache* soc_new(size_t alignment, size_t objectSize, int preallocCount);

#define __SOC_CALC_ALIGNMENT(alignment) MAX(alignof(struct soc_free_node), (alignment))
#define __SOC_CALC_CHUNK_SIZE(chunkSize, alignment) ROUND_UP((chunkSize), __SOC_CALC_ALIGNMENT((alignment)))
#define __SOC_CALC_OBJECT_SIZE(objectSize, alignment) ROUND_UP(MAX((objectSize), SOC_MIN_OBJECT_SIZE), __SOC_CALC_ALIGNMENT((alignment)))
#define __SOC_DEFINE_ADVANCED(_MEOOOW_UWU, _name, _chunkSize, _alignment, _objectSize) \
  static struct small_object_cache ___ ## _name = (struct small_object_cache) { \
    .isStatic = true, \
    .alignment = __SOC_CALC_ALIGNMENT((_alignment)), \
    .objectSize = __SOC_CALC_OBJECT_SIZE((_objectSize), (_alignment)), \
    .chunkSize = __SOC_CALC_CHUNK_SIZE((_chunkSize), (_alignment)), \
    .fullChunksList = LIST_HEAD_INIT(___ ## _name.fullChunksList), \
    .partialChunksList = LIST_HEAD_INIT(___ ## _name.partialChunksList), \
  }; static_assert(__SOC_CALC_CHUNK_SIZE((_chunkSize), (_alignment)) / __SOC_CALC_OBJECT_SIZE((_objectSize), (_alignment)) >= SOC_MIN_OBJECT_COUNT); \
  _MEOOOW_UWU small_object_cache* const _name = &___ ## _name
#define __SOC_DEFINE(_MEOOOW_UWU, _name, _chunkSize, _type) __SOC_DEFINE_ADVANCED(_MEOOOW_UWU, _name, _chunkSize, alignof(_type), sizeof(_type))

#define SOC_DEFINE(_name, _chunkSize, _type) __SOC_DEFINE(struct, _name, _chunkSize, _type)
#define SOC_DEFINE_ADVANCED(_name, _chunkSize, _alignment, _objectSize) __SOC_DEFINE_ADVANCED(struct, _name, _chunkSize, _alignment, _objectSize)
#define SOC_DEFINE_STATIC(_name, _chunkSize, _type) __SOC_DEFINE(static struct, _name, _chunkSize, _type)
#define SOC_DEFINE_STATIC_ADVANCED(_name, _chunkSize, _alignment, _objectSize) __SOC_DEFINE_ADVANCED(static struct, _name, _chunkSize, _alignment, _objectSize)

#define SOC_DECLARE(_name, _type) extern struct small_object_cache* const _name

void* soc_alloc(struct small_object_cache* self);
void soc_dealloc(struct small_object_cache* self, void* ptr);

// Only used in btree to avoid dead infinite loops
// because recursive relying on each other
void* soc_alloc_explicit(struct small_object_cache* self, struct soc_chunk** chunk);
void soc_dealloc_explicit(struct small_object_cache* self, struct soc_chunk* chunk, void* ptr);

void soc_free(struct small_object_cache* self);

#endif

