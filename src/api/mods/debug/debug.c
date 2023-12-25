#include <threads.h>

#include "FluffyHeap/FluffyHeap.h"
#include "FluffyHeap/mods/debug.h"

#include "logger/logger.h"
#include "api/api.h"
#include "api/mods/debug/debug.h"
#include "config.h"
#include "context.h"
#include "managed_heap.h"
#include "macros.h"

// Debug mode
DEFINE_LOGGER(debug_logger, "Debug Mod");

#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&debug_logger)

int debug_check_flags(uint32_t flags) {
  UNUSED(flags);
  return 0;
}

bool debug_can_do_check() {
  if (!managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].enabled)
    return false;
  
  return managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].data.debugMod.verbosity > MOD_DEBUG_SILENT;
}

bool debug_can_do_api_tracing() {
  if (!managed_heap_current)
    return false;
  
  if (!managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].enabled)
    return false;
  
  return managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].data.debugMod.verbosity >= MOD_DEBUG_ULTRA_VERBOSE;
}

int debug_init(struct api_mod_state* self) {
  // Build configuration overrides app request
  self->data.debugMod.panicOnError = IS_ENABLED(CONFIG_MOD_DEBUG_DEFAULT_PANIC) || (!!(self->flags & FH_MOD_DEBUG_DONT_KEEP_GOING));
  self->data.debugMod.verbosity = MOD_DEBUG_ULTRA_VERBOSE;
  pr_info("Debug mod enabled!");
  return 0;
}

void debug_cleanup(struct api_mod_state* self) {
  UNUSED(self);
  return;
}

bool debug_can_panic_on_warn() {
  return managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].data.debugMod.panicOnError;
}


