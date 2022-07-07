#ifndef header_1657017489_ded80d02_30e6_4bac_a43f_2ffef8dd9335_old_collector_h
#define header_1657017489_ded80d02_30e6_4bac_a43f_2ffef8dd9335_old_collector_h

#include <stdbool.h>

struct gc_state;
bool gc_old_pre_collect(struct gc_state* state);

// Start the collection
void gc_old_collect(struct gc_state* state);

void gc_old_post_collect(struct gc_state* state);

#endif

