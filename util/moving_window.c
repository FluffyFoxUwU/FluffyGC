#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "moving_window.h"

struct moving_window* moving_window_new(size_t entrySize, unsigned int maxCount) {
  struct moving_window* self = malloc(sizeof(*self) + entrySize * maxCount);
  if (!self)
    return NULL;
  
  *self = (struct moving_window) {
    .entrySize = entrySize,
    .maxEntryCount = maxCount,
    .entrySize = entrySize
  };
  return self;
}

void moving_window_free(struct moving_window* self) {
  free(self);
}

void moving_window_append(struct moving_window* self, void* entry) {
  memcpy(self->data + self->nextWindowIndex * self->entrySize, entry, self->entrySize);
  
  self->nextWindowIndex = (self->nextWindowIndex + 1) % self->maxEntryCount;
  if (self->entryCount < self->maxEntryCount)
    self->entryCount++;
}

bool moving_window_next(struct moving_window* self, struct moving_window_iterator* iterator) {
  // Foxie has iterated everything there is in the window
  if (iterator->numberOfIterations == self->entryCount)
    return false;
  
  iterator->current = self->data + ((self->nextWindowIndex + iterator->numberOfIterations) % self->maxEntryCount) * self->entrySize;
  iterator->numberOfIterations++;
  return true;
}

