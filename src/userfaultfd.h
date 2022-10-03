#ifndef _headers_1664803954_FluffyGC_userfaultfd
#define _headers_1664803954_FluffyGC_userfaultfd

#include <stdbool.h>

int uffd_create_descriptor(int flags);

bool uffd_is_available();
bool uffd_is_supported();

#endif

