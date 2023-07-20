#ifndef _headers_1689412335_FluffyGC_debug
#define _headers_1689412335_FluffyGC_debug

#include <stdint.h>

#include "hook/hook.h"
#include "FluffyHeap.h"
#include "util/util.h"

struct api_mod_state;

int api_mod_debug_check_flags(uint32_t flags);
int api_mod_debug_init(NONNULLABLE(struct api_mod_state*) self);
void api_mod_debug_cleanup(NONNULLABLE(struct api_mod_state*) self);

HOOK_FUNCTION_DECLARE(, size_t, api_mod_debug_hook_fh_array_get_length, __FLUFFYHEAP_NONNULL(fh_array*), self);

#endif

