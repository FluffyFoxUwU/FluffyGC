#ifndef UWU_A7F05A83_764D_4B35_915C_57321065BE4A_UWU
#define UWU_A7F05A83_764D_4B35_915C_57321065BE4A_UWU

#include <stddef.h>

#include <flup/util/refcount.h>

// Simplified system where programs
// manage the descriptors' memory instead
// of GC system, while GC system only
// queue the descriptor to be free'd

struct field {
  size_t offset;
};

struct descriptor {
  size_t objectSize;
  size_t fieldCount;
  
  // Intend to cover structures which has GC-able pointers
  // in flexible array at the end of structs
  bool hasFlexArrayField;
  struct field fields[];
};

void descriptor_init_object(struct descriptor* desc, size_t extraSize, void* data);

#endif
