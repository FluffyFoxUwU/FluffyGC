#ifndef UWU_C55EE697_F5F2_40C2_8C38_38BA1C3501CB_UWU
#define UWU_C55EE697_F5F2_40C2_8C38_38BA1C3501CB_UWU

#include <stddef.h>

typedef struct fluffygc_descriptor {
  size_t objectSize;
  size_t fieldCount;
  bool hasFlexArrayField;
  size_t fields[];
} fluffygc_descriptor;

#endif
