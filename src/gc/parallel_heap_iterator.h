#ifndef header_1659618962_78b89435_b8c9_4115_b287_ed200c146f48_parallel_sweeper_h
#define header_1659618962_78b89435_b8c9_4115_b287_ed200c146f48_parallel_sweeper_h

#include <stdbool.h>

struct gc_state;
struct object_info;
typedef bool (^gc_parallel_heap_iterator)(struct object_info* info, int idx);

bool gc_parallel_heap_iterator_do(struct gc_state* gcState, bool isYoung, gc_parallel_heap_iterator consumer, int* lastPos);

#endif

