#ifndef _headers_1683212706_FluffyGC_nop
#define _headers_1683212706_FluffyGC_nop

#include "gc/gc.h"

struct gc_hooks* gc_nop_new(int flags);
int gc_nop_generation_count(int flags);
bool gc_nop_use_fast_on_gen(int flags, int genID);

#endif

