#ifndef UWU_BB5F356F_7184_454E_8F5E_E2EF790C4B64_UWU
#define UWU_BB5F356F_7184_454E_8F5E_E2EF790C4B64_UWU

// Basically an alias of struct heap*
// its present to avoid potential clashing with Lua used names
typedef struct fgc_heap* fgc_heap;

void fgc_block_gc(fgc_heap* self);
void fgc_unblock_gc(fgc_heap* self);

#endif
