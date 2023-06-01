#ifndef _headers_1685458233_FluffyGC_generic
#define _headers_1685458233_FluffyGC_generic

#include <stddef.h>

struct generation;

// These are generic and can work with any amount of generations
// and also these merely just each of 3 full gc blocking stages
void gc_generic_compact(struct generation*);
size_t gc_generic_collect(struct generation*);
int gc_generic_mark(struct generation*);

size_t gc_generic_collect_and_mark(struct generation*);

#endif

