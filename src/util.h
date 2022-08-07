#ifndef header_1655989530_util_h
#define header_1655989530_util_h

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

bool util_atomic_add_if_less_uint(volatile atomic_uint* data, unsigned int n, unsigned int max, unsigned int* result);
bool util_atomic_add_if_less_int(volatile atomic_int* data, int n, int max, int* result);

int util_atomic_min_int(volatile atomic_int* object, int newVal);

bool util_vasprintf(char** result, const char* fmt, va_list arg);

void util_set_thread_name(const char* name);

int util_get_core_count();

#endif

