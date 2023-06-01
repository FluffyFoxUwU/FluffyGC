#ifndef _headers_1685539816_FluffyGC_gc_flags
#define _headers_1685539816_FluffyGC_gc_flags

/*
GC flags allocation
bit 0-55 free to use by GC algorithmns (56 bits)
bit 56-63 reserved for this global GC flags (8 bits)
*/

#include <stdint.h>

typedef uint64_t gc_flags;

// Serial GC flags
#define SERIAL_GC_USE_2_GENERATIONS        (1 << 0)

#endif

