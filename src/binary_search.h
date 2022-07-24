#ifndef header_1658239133_3b0e84ef_9201_478e_b969_2b6ee44b9da4_binary_search_h
#define header_1658239133_3b0e84ef_9201_478e_b969_2b6ee44b9da4_binary_search_h

// Generic binary search
#include <stddef.h>

typedef int (*binary_search_compare_func)(const void* _a, const void* _b);
int binary_search_do(void* array, int count, size_t elementSize, binary_search_compare_func cmp, void* value);

#endif

