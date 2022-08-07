#ifndef header_1659618685_c2c7d076_37a9_4fe5_a4d1_a5f3310eede0_parallel_marker_h
#define header_1659618685_c2c7d076_37a9_4fe5_a4d1_a5f3310eede0_parallel_marker_h

#include <stdbool.h>

struct gc_state;
void gc_parallel_marker(struct gc_state* gcState, bool isYoung);

#endif

