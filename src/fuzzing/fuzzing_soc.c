#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory/soc.h"
#include "config.h"

int fuzzing_soc(const void* data, size_t size) {
  if (size < sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t))
    return 0;
  
  const void* dataEnd = data + size;
  
  size_t objSize = *(const uint16_t*) data + 1;
  data += 2;
    
  size_t alignment = *(const uint16_t*) data + 1;
  data += 2;
  
  int reserveCount = *(const uint8_t*) data;
  data += 1;
  
  int pattern = *(const uint8_t*) data;
  data += 1;
  
  // 256 bytes maximum alignment allowed
  alignment = 2 << (alignment % 8);
  
  struct small_object_cache* cache = soc_new(alignment, objSize, reserveCount);
  
  static void* pointers[1 << 16] = {};
  static struct soc_chunk* chunks[1 << 16] = {};
  memset(pointers, 0, sizeof(pointers));
  memset(chunks, 0, sizeof(chunks));
  
  while (data < dataEnd) {
    uint16_t id = *(const uint8_t*) data + (*(const uint8_t*) data << 8);
    if (pointers[id]) {
      if (IS_ENABLED(CONFIG_FUZZ_SOC_USE_IMPLICIT))
        soc_dealloc(cache, pointers[id]);
      else
        soc_dealloc_explicit(cache, chunks[id], pointers[id]);
      pointers[id] = NULL;
    } else {
      if (IS_ENABLED(CONFIG_FUZZ_SOC_USE_IMPLICIT))
        pointers[id] = soc_alloc(cache);
      else
        pointers[id] = soc_alloc_explicit(cache, &chunks[id]);
      memset(pointers[id], pattern, objSize);
    }
    data += 2;
  }
  
  soc_free(cache);
  return 0;
}
