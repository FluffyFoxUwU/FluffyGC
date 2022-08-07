#ifndef header_1659527894_d9f63d1e_daf2_41b6_9b9d_33ac0dedc5bb_common_h
#define header_1659527894_d9f63d1e_daf2_41b6_9b9d_33ac0dedc5bb_common_h

#include <stdbool.h>

struct gc_state;
void gc_clear_non_strong_refs(struct gc_state* gcState, bool isYoung);

#endif

