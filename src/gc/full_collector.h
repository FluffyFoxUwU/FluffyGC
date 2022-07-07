#ifndef header_1657067883_9ff437e6_abeb_4f0e_bc19_0919e13f0f60_full_collector_h
#define header_1657067883_9ff437e6_abeb_4f0e_bc19_0919e13f0f60_full_collector_h

#include <stdbool.h>

struct gc_state;
bool gc_full_pre_collect(struct gc_state* state);

// Start the collection
void gc_full_collect(struct gc_state* state);

void gc_full_post_collect(struct gc_state* state);

#endif

