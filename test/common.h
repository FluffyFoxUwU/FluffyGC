#ifndef UWU_BB5F356F_7184_454E_8F5E_E2EF790C4B64_UWU
#define UWU_BB5F356F_7184_454E_8F5E_E2EF790C4B64_UWU

#include <stddef.h>

// Basically an alias of struct heap*
// its present to avoid potential clashing with Lua used names
//
// casted to struct heap*
typedef struct fgc_heap* fgc_heap;

// A pointer to the object itself
// (must have associated root reference
// so GC dont nuke them)
//
// casted to struct arena_block*
typedef struct fgc_object* fgc_object;

// A root reference handle to a fgc_object
// you can get fgc_object from this without
// any GC intervention as there no moving
// GC to change pointer to it
//
// casted to struct root_ref*
typedef struct fgc_root_ref* fgc_root_ref;

#include "object/descriptor.h"
typedef struct descriptor fgc_descriptor;
typedef struct field fgc_field;

void fgc_block_gc(fgc_heap* self);
void fgc_unblock_gc(fgc_heap* self);

fgc_root_ref* fgc_new_object(fgc_heap* self, size_t size);
fgc_root_ref* fgc_new_object_with_descriptor(fgc_heap* self, fgc_descriptor* desc, size_t extraSize);

fgc_root_ref* fgc_dup_root(fgc_heap* self, fgc_root_ref* ref);
void fgc_unref_root(fgc_heap* self, fgc_root_ref* ref);

fgc_root_ref* fgc_read_object(fgc_heap* self, fgc_object* obj, size_t offset);
void fgc_write_object(fgc_heap* self, fgc_object* obj, size_t offset, fgc_object* newObj);

#endif
