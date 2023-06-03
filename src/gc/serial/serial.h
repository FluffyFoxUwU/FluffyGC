#ifndef _headers_1685457249_FluffyGC_serial
#define _headers_1685457249_FluffyGC_serial

#include <stdint.h>

#include "gc/gc_flags.h"

struct gc_ops* gc_serial_new(gc_flags flags);
int gc_serial_generation_count(gc_flags flags);
bool gc_serial_use_fast_on_gen(gc_flags flags, int genID);

#endif

