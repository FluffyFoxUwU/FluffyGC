#ifndef UWU_FCD1D747_864E_4042_B646_947A6330071F_UWU
#define UWU_FCD1D747_864E_4042_B646_947A6330071F_UWU

#include <stddef.h>

// A moving window of entries (where oldest one get pushed away)

struct moving_window {
  size_t entrySize;
  unsigned int nextWindowIndex;
  unsigned int entryCount;
  unsigned int maxEntryCount;
  
  char data alignas(max_align_t)[];
};

struct moving_window_iterator {
  unsigned int numberOfIterations;
  void* current;
};

struct moving_window* moving_window_new(size_t entrySize, unsigned int maxCount);
void moving_window_free(struct moving_window* self);

void moving_window_append(struct moving_window* self, void* entry);
bool moving_window_next(struct moving_window* self, struct moving_window_iterator* iterator);

#endif
