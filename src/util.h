#ifndef header_1655989530_util_h
#define header_1655989530_util_h

#include <stdbool.h>
#include <stdatomic.h>

bool util_atomic_add_if_less_uint(volatile atomic_uint* data, unsigned int n, unsigned int max, unsigned int* result);
bool util_atomic_add_if_less_int(volatile atomic_int* data, int n, int max, int* result);

#endif

