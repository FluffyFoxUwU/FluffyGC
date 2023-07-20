#ifndef _headers_1689412335_FluffyGC_debug
#define _headers_1689412335_FluffyGC_debug

#include <stdint.h>

#include "hook/hook.h"
#include "FluffyHeap.h"
#include "util/util.h"

struct api_mod_state;

enum mod_debug_verbosity {
  MOD_DEBUG_SILENT,
  MOD_DEBUG_WARN_ONLY,
  MOD_DEBUG_VERBOSE,
  MOD_DEBUG_ULTRA_VERBOSE
};

struct debug_mod_state {
  enum mod_debug_verbosity verbosity;
  bool panicOnWarn;
};

int api_mod_debug_check_flags(uint32_t flags);
int api_mod_debug_init(NONNULLABLE(struct api_mod_state*) self);
void api_mod_debug_cleanup(NONNULLABLE(struct api_mod_state*) self);

HOOK_FUNCTION_DECLARE(, size_t, api_mod_debug_hook_fh_array_get_length, __FLUFFYHEAP_NONNULL(fh_array*), self);

HOOK_FUNCTION_DECLARE(, __FLUFFYHEAP_NULLABLE(fluffyheap*), api_mod_debug_hook_fh_new_head, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams);
HOOK_FUNCTION_DECLARE(, __FLUFFYHEAP_NULLABLE(fluffyheap*), api_mod_debug_hook_fh_new_tail, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams);

#endif

