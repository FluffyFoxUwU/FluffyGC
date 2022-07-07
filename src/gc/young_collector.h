#ifndef header_1656479010_f30e53ef_46da_41b9_9dc5_2bd9bba086b4_young_collector_h
#define header_1656479010_f30e53ef_46da_41b9_9dc5_2bd9bba086b4_young_collector_h

#include <stdbool.h>

struct gc_state;
bool gc_young_pre_collect(struct gc_state* state);

// Start the collection
void gc_young_collect(struct gc_state* state);

void gc_young_post_collect(struct gc_state* state);

#endif

