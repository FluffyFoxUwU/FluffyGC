#ifndef UWU_A7F05A83_764D_4B35_915C_57321065BE4A_UWU
#define UWU_A7F05A83_764D_4B35_915C_57321065BE4A_UWU

#include <stddef.h>

struct field {
  size_t offset;
};

struct descriptor {
  size_t objectSize;
  size_t fieldCount;
  struct field fields[];
};

#endif
