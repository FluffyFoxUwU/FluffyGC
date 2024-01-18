#ifndef header_1703493606_1194a5e4_6961_4032_8032_79b2f3de1c00_arrays_h
#define header_1703493606_1194a5e4_6961_4032_8032_79b2f3de1c00_arrays_h

#include <stddef.h>

#include "FluffyHeap/FluffyHeap.h"
#include "hook/hook.h"

HOOK_FUNCTION_DECLARE(, ssize_t, debug_hook_fh_array_get_length_head, __FLUFFYHEAP_NONNULL(fh_array*), self);
HOOK_FUNCTION_DECLARE(, ssize_t, debug_hook_fh_array_calc_offset_head, __FLUFFYHEAP_NONNULL(fh_array*), self, size_t, index);

#endif

