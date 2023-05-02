#ifndef _headers_1682059498_FluffyGC_gc
#define _headers_1682059498_FluffyGC_gc

#include "object/object.h"

struct gc_struct {
  int nothing;
};

// Barrier must can be called on both GC blocked and GC unblocked state
// and must be thread safe
void gc_write_barrier(struct object* object);
void gc_read_barrier(struct object* object);

// 0 stands for young GC
// 1 stands for old GC
void gc_start(int generation);

// GC may decide what to do like to trigger concurrent GC, attach
// metadata or stuff
void gc_on_alloc(struct object* object);

// This must strictly only clean metadata
void gc_on_dealloc(struct object* object);

#endif

