#ifndef header_1655989530_util_h
#define header_1655989530_util_h

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "bits.h"

bool util_atomic_add_if_less_uint(volatile atomic_uint* data, unsigned int n, unsigned int max, unsigned int* result);
bool util_atomic_add_if_less_int(volatile atomic_int* data, int n, int max, int* result);

int util_atomic_min_int(volatile atomic_int* object, int newVal);

bool util_vasprintf(char** result, const char* fmt, va_list arg);

void util_set_thread_name(const char* name);

int util_get_core_count();

// Return 0 on failure and sets errno
size_t util_get_pagesize();

// Result can be unmapped via `munmap`
// Sets errno on error
// Errors:
// -ENOSYS: Anonymous mmap is not enabled
// -EINVAL: Invalid arguments
// -ENOMEM: No memory available
void* util_mmap_anonymous(size_t size, int protection);

#define swap(a, b) do { \
  typeof(a) _UwU_Swap_a = (a); \
  typeof(b) _UwU_Swap_b = (b); \
  a = _UwU_Swap_b; \
  b = _UwU_Swap_a; \
} while(0)

#define indexof(array, member) ((size_t) (member - array) / sizeof(*array))

typedef _Atomic(void*) atomic_untyped_ptr;
typedef struct {
  void* ptr;
} untyped_ptr;

void util_nearest_power_of_two(size_t* size);

#define UTIL_MEM_ZERO BIT(1)
void* util_realloc(void* ptr, size_t oldSize, size_t newSize, unsigned long flags);
void* util_malloc(size_t newSize, unsigned long flags);

#define UTIL_DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define UTIL_DIV_ROUND_DOWN(n, d) ((n) / (d))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define container_of(ptr, type, member) ((type*) ((void*)ptr - offsetof(type,member)))

#endif

