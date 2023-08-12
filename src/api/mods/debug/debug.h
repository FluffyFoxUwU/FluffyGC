#ifndef _headers_1689412335_FluffyGC_debug
#define _headers_1689412335_FluffyGC_debug

#include <stdint.h>

#include "attributes.h"

struct api_mod_state;

enum mod_debug_verbosity {
  MOD_DEBUG_SILENT,
  MOD_DEBUG_WARN_ONLY,
  MOD_DEBUG_ULTRA_VERBOSE
};

struct debug_mod_state {
  enum mod_debug_verbosity verbosity;
  bool panicOnWarn;
};

int debug_check_flags(uint32_t flags);
int debug_init(struct api_mod_state* self);
void debug_cleanup(struct api_mod_state* self);

bool debug_can_do_check();
bool debug_can_do_api_verbose();

ATTRIBUTE_PRINTF(1, 2)
void debug_info(const char* msg, ...);

ATTRIBUTE_PRINTF(1, 2)
void debug_warn(const char* msg, ...);

#endif

