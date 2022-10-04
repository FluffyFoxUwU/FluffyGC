#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "util.h"
#include "config.h"

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
