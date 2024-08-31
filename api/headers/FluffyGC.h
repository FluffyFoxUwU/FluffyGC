#ifndef UWU_A21AD3F1_5382_4BDB_ABF2_23BC2B6F416A_UWU
#define UWU_A21AD3F1_5382_4BDB_ABF2_23BC2B6F416A_UWU

#include <stddef.h>

#define FLUFFYGC_EXPORT \
  [[gnu::visibility("default")]] \
  extern

typedef struct fluffygc_state fluffygc_state;

FLUFFYGC_EXPORT
fluffygc_state* fluffygc_new(size_t heapSize);

FLUFFYGC_EXPORT
void fluffygc_free(fluffygc_state* self);

#endif
