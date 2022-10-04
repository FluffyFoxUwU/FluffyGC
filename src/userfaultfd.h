#ifndef _headers_1664803954_FluffyGC_userfaultfd
#define _headers_1664803954_FluffyGC_userfaultfd

#include <stdbool.h>

// Negative on error else file descriptor
// Errors:
// -EINVAL: Invalid flags
// -ENOSYS: userfaultfd support is disabled or not compiled in
int uffd_create_descriptor(int flags);

// Whether uffd support is available (support is compiled in)
bool uffd_is_available();

// Whether current kernel support uffd and do requirement checks
// Note: dont scare calling this function alot the result
//       is cached
bool uffd_is_supported();

#endif

