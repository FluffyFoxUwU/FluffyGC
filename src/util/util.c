#include "config.h"
#if !IS_ENABLED(CONFIG_STRICTLY_POSIX)
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "bug.h"

bool util_atomic_add_if_less_uint(volatile atomic_uint* data, unsigned int n, unsigned int max, unsigned int* result) {
  unsigned int new;
  unsigned int old = atomic_load(data);
  do {
    new = old + n;
     
    if (new >= max)
      return false;
  } while (!atomic_compare_exchange_weak(data, &old, new));

  *result = old;
  return true;
}

bool util_atomic_add_if_less_uintptr(volatile atomic_uintptr_t* data, uintptr_t n, uintptr_t max, uintptr_t* result) {
  uintptr_t new;
  uintptr_t old = atomic_load(data);
  do {
    new = old + n;
    
    if (new >= max)
      return false;
  } while (!atomic_compare_exchange_weak(data, &old, new));

  *result = old;
  return true;
}

int util_atomic_min_int(volatile atomic_int* object, int newVal) {
  int new;
  int old = atomic_load(object);
  do {
    if (newVal < old)
      new = newVal;
    else
      return old;
  } while (!atomic_compare_exchange_weak(object, &old, new));

  return old;
}

bool util_atomic_add_if_less_int(volatile atomic_int* data, int n, int max, int* result) {
  int new;
  int old = atomic_load(data);
  do {
    new = old + n;
    
    if (new >= max)
      return false;
  } while (!atomic_compare_exchange_weak(data, &old, new));

  *result = old;
  return true;
}

bool util_vasprintf(char** result, const char* fmt, va_list arg) {
  size_t buflen = vsnprintf(NULL, 0, fmt, arg);
  *result = malloc(buflen + 1);
  if (*result == NULL)
    return false;

  vsnprintf(*result, buflen, fmt, arg);
  return true;
}

void util_set_thread_name(const char* name) {
  prctl(PR_SET_NAME, name);
}

int util_get_core_count() {
  // Casting fine for 99.999% of the time
  // due why would 2 billions cores exists
  return (int) sysconf(_SC_NPROCESSORS_ONLN);
}

size_t util_get_pagesize() {
  long result = sysconf(_SC_PAGESIZE);
  
  if (result == -1)
    return 0;
  
  return result;
}

void* util_mmap_anonymous(size_t size, int protection) {
# if IS_ENABLED(CONFIG_ALLOW_USAGE_OF_ANONYMOUS_MMAP)
  void* addr = mmap(NULL, size, protection, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (addr == MAP_FAILED) {
    switch (errno) {
      case ENOMEM:
        errno = ENOMEM;
        break;
      case EINVAL:
      default:
        errno = EINVAL;
        break;
    }
  }
  return addr == MAP_FAILED ? NULL : addr;
# else
  errno = ENOSYS;
  return NULL;
# endif
}

void util_nearest_power_of_two(size_t* size) {
  size_t x = *size;
  *size = 1;
  while(*size < x && *size < SIZE_MAX)
    *size <<= 1;
}

bool util_is_power_of_two(size_t size) {
  size_t tmp = size;
  util_nearest_power_of_two(&tmp);
  return tmp == size;
}

void* util_malloc(size_t size, unsigned long flags) {
  void* ptr = malloc(size);
  if (!ptr)
    return NULL;
  
  WARN_ON(size == 0);
  if (flags & UTIL_MEM_ZERO)
    memset(ptr, 0, size);
  return ptr;
}

void* util_realloc(void* ptr, size_t oldSize, size_t newSize, unsigned long flags) {
  ptr = realloc(ptr, newSize);
  if (!ptr)
    return NULL;
  
  if ((flags & UTIL_MEM_ZERO) && newSize > oldSize)
    memset(ptr + oldSize, 0, newSize - oldSize);
  
  return ptr;
}

// Less than equal binary search
// https://stackoverflow.com/a/28309550/13447666
const uintptr_t* util_find_smallest_but_larger_or_equal_than(const uintptr_t* array, size_t count, uintptr_t search) {
  const uintptr_t* min =  array;
  const uintptr_t* max =  &array[count - 1];
  const uintptr_t* arrayEnd =  &array[count - 1];
  
  if (count <= 0)
    return NULL;
  if (search <= array[0])
    return array;
  
  while (min <= max) {
    const uintptr_t* mid = min + ((max - min) / 2);
    if (*mid < search)
      min = mid + 1;
    else if (*mid > search)
      max = mid - 1;
    else
      return mid;
  }
  
  if (min > arrayEnd)
    return NULL; // Not found
  return min < max ? min : max + 1;
}

void util_shift_array(void* start, size_t offset, size_t size) {
  memmove(start + (offset * size), start, size);
}
