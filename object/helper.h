#ifndef UWU_CC3356B9_7842_4A43_A49C_F938344D5240_UWU
#define UWU_CC3356B9_7842_4A43_A49C_F938344D5240_UWU

#include <stddef.h>

#include "heap/heap.h"
#include "memory/arena.h"

// A little helpers to read/write refs in object
// and triggering necessary barriers

void object_helper_write_ref(struct heap* heap, struct arena_block* block, size_t offset, struct arena_block* newBlock);
struct root_ref* object_helper_read_ref(struct heap* heap, struct arena_block* block, size_t offset);

#endif
