#ifndef _headers_1683212706_FluffyGC_nop
#define _headers_1683212706_FluffyGC_nop

#include <stddef.h>
#include <stdint.h>

#include "gc/gc_flags.h"
#include "gc/gc.h"

struct gc_ops* gc_nop_new(gc_flags flags);
int gc_nop_generation_count(gc_flags flags);
bool gc_nop_use_fast_on_gen(gc_flags flags, int genID);
int gc_nop_mark(struct generation* gen);
size_t gc_nop_collect(struct generation* gen);
void gc_nop_compact(struct generation* gen);
void gc_nop_start_cycle(struct generation*);
void gc_nop_end_cycle(struct generation*);
void gc_nop_free(struct gc_ops*);
size_t gc_nop_preferred_promotion_size(gc_flags flags, int genID);
int gc_nop_preferred_promotion_age(gc_flags flags, int genID);

#endif

