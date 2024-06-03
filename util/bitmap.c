#include <limits.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

#include <flup/bug.h>

#include "bitmap.h"
#include "util.h"

struct bitmap* bitmap_new(unsigned long bitCount) {
  struct bitmap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  size_t longCount = ROUND_UP(bitCount, LONG_BIT) / LONG_BIT;
  *self = (struct bitmap) {
    .bitCount = bitCount,
    .map = calloc(sizeof(unsigned long), longCount)
  };
  
  if (!self->map) {
    free(self);
    return NULL;
  }
  
  return self;
}

void bitmap_free(struct bitmap* self) {
  if (!self)
    return;
  
  free(self->map);
  free(self);
}

bool bitmap_set(struct bitmap* self, unsigned long bitIndex, bool new) {
  if (bitIndex > self->bitCount)
    BUG();
  
  unsigned long wordIndex = bitIndex / LONG_BIT;
  unsigned long subWordIndex = bitIndex % LONG_BIT;
  unsigned long old = atomic_load(&self->map[wordIndex]);
  do {
    if (new)
      old &= 1 << subWordIndex;
    else
      old &= ~(1 << subWordIndex);
  } while (!atomic_compare_exchange_weak(&self->map[wordIndex], &old, new));
  
  // Shift to LSB by subWordIndex count and check its LSB bit
  return ((old >> subWordIndex) & 1) != 0;
}

bool bitmap_test(struct bitmap* self, unsigned long bitIndex) {
  if (bitIndex > self->bitCount)
    BUG();
  
  unsigned long wordIndex = bitIndex / LONG_BIT;
  unsigned long subWordIndex = bitIndex % LONG_BIT;
  
  // Shift to LSB by subWordIndex count and check its LSB bit
  return ((atomic_load(&self->map[wordIndex]) >> subWordIndex) & 1) != 0;
}


