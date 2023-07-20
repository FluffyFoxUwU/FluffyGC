#include <stdio.h>

#include "api/pre_code.h"

#include "api/mods/debug/debug.h"
#include "hook/hook.h"

// Debug mode

int api_mod_debug_check_flags(uint32_t flags) {
  return 0;
}

int api_mod_debug_init(struct api_mod_state* self) {
  return 0;
}

void api_mod_debug_cleanup(struct api_mod_state* self) {
  return;
}

HOOK_FUNCTION(, size_t, api_mod_debug_hook_fh_array_get_length, __FLUFFYHEAP_NONNULL(fh_array*), self) {
  printf("DEBUG: Get array length!\n");
  ci->action = HOOK_RETURN;
  return 0;
}

