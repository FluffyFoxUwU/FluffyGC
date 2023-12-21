#ifndef header_1655989530_util_h
#define header_1655989530_util_h

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bits.h"

#define UTIL_MAX_THREAD_NAME_LEN (128)

// return true if assigned
bool util_atomic_add_if_less_uint(volatile atomic_uint* data, unsigned int n, unsigned int max, unsigned int* result);
bool util_atomic_add_if_less_uintptr(volatile atomic_uintptr_t* data, uintptr_t n, uintptr_t max, uintptr_t* result);
bool util_atomic_add_if_less_size_t(volatile atomic_size_t* data, size_t n, size_t max, size_t* result);
bool util_atomic_add_if_less_int(volatile atomic_int* data, int n, int max, int* result);
int util_atomic_min_int(volatile atomic_int* object, int newVal);

bool util_vasprintf(char** result, const char* fmt, va_list arg);

void util_set_thread_name(const char* name);
const char* util_get_thread_name();

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

#define indexof(array, member) ((size_t) (((uintptr_t) (member)) - ((uintptr_t) array)) / sizeof(*array))

typedef _Atomic(void*) atomic_untyped_ptr;
typedef struct {
  void* ptr;
} untyped_ptr;

void util_nearest_power_of_two(size_t* size);
bool util_is_power_of_two(size_t size);

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_DOWN(n, d) ((n) / (d))

#define ROUND_UP(n, d) (DIV_ROUND_UP((n), (d)) * (d))
#define ROUND_DOWN(n, d) (DIV_ROUND_DOWN((n), (d)) * (d))

#define PTR_ALIGN(ptr, alignment) ((typeof(ptr)) ROUND_UP((uintptr_t) (ptr), (alignment)))

#define UTIL_MEM_ZERO BIT (1)
void* util_realloc(void* ptr, size_t oldSize, size_t newSize, unsigned long flags);
void* util_malloc(size_t newSize, unsigned long flags);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef __GNUC__
# define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#else
# define container_of(ptr, type, member) ((type *)( (char *)__mptr - offsetof(type,member) )) 
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define FIELD_SIZEOF(t, f) (sizeof(((t*) NULL)->f))

const uintptr_t* util_find_smallest_but_larger_or_equal_than(const uintptr_t* array, size_t count, uintptr_t search);
void util_shift_array(void* start, size_t offset, size_t size);

// Relaxed aligned_alloc
void* util_aligned_alloc(size_t alignment, size_t size);

static inline bool util_prefixed_by(const char* prefix, const char* str) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

void util_sleep(unsigned int secs);
void util_msleep(unsigned int microsecs);
void util_usleep(unsigned long milisecs);
void util_nanosleep(unsigned long nanosecs);

int util_sleep_until(struct timespec* ts);

double util_get_realtime_time();
double util_get_monotonic_time();
double util_get_total_cpu_time();
double util_get_thread_cpu_time();

void util_add_timespec(struct timespec* ts, double offset);
double util_timespec_to_float(struct timespec* ts);
struct timespec util_relative_to_abs(clockid_t clock, double offset);

#define __stringify_1(x...)	#x
#define stringify(x...)	__stringify_1(x)

#if __clang__
# define NONNULLABLE(t) t _Nonnull
# define NULLABLE(t) t _Nullable
#elif __GNUC__
# define NONNULLABLE(t) t __attribute__((nonnull))
# define NULLABLE(t) t
#endif

#endif

