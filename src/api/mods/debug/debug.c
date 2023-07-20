#include <stdarg.h>
#include <stdio.h>
#include <threads.h>

#include "api/pre_code.h"

#include "api/api.h"
#include "api/mods/debug/debug.h"
#include "config.h"
#include "hook/hook.h"
#include "context.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "FluffyHeap.h"
#include "managed_heap.h"
#include "panic.h"

// Debug mode

int api_mod_debug_check_flags(uint32_t flags) {
  return 0;
}

int api_mod_debug_init(struct api_mod_state* self) {
  self->data.debugMod.panicOnWarn = IS_ENABLED(CONFIG_MOD_DEBUG_PANIC);
  self->data.debugMod.verbosity = MOD_DEBUG_ULTRA_VERBOSE;
  return 0;
}

void api_mod_debug_cleanup(struct api_mod_state* self) {
  return;
}

static void api_mod_debug_warn(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  
  static thread_local char buffer[512 * 1024] = {};
  vsnprintf(buffer, sizeof(buffer), msg, args);
  
  if (managed_heap_current->api.state->modStates[FH_MOD_DEBUG].data.debugMod.panicOnWarn)
    panic("Ouch -w-: %s", buffer);
  else
    fprintf(stderr, "%s", buffer);
  va_end(args);
  
}

HOOK_FUNCTION(, size_t, api_mod_debug_hook_fh_array_get_length, __FLUFFYHEAP_NONNULL(fh_array*), self) {
  if (managed_heap_current->api.state->modStates[FH_MOD_DEBUG].data.debugMod.verbosity == MOD_DEBUG_SILENT) {
    ci->action = HOOK_CONTINUE;
    return 0;
  }
  
  context_block_gc();
  struct descriptor* desc = atomic_load(&INTERN(self)->obj)->movePreserve.descriptor;
  if (desc->type != OBJECT_ARRAY)
    api_mod_debug_warn("[DEBUG] fh_array_get_length: Getting length on non array object!!\n");
  context_unblock_gc();
  ci->action = HOOK_CONTINUE;
  return 0;
}

// Hook to always enable
// debug mod if needed

static thread_local uint32_t prevDebugFlags;
HOOK_FUNCTION(, __FLUFFYHEAP_NULLABLE(fluffyheap*), api_mod_debug_hook_fh_new_head, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams) {
  if (IS_ENABLED(CONFIG_MOD_DEBUG_ALWAYS_ENABLE)) {
    prevDebugFlags = fh_get_flags(FH_MOD_DEBUG);
    
    int ret = fh_enable_mod(FH_MOD_DEBUG, 0);
    BUG_ON(ret == -EINVAL);
    printf("[DEBUG] Always enabled activated!\n");
  }
  
  ci->action = HOOK_CONTINUE;
  return NULL;
}

HOOK_FUNCTION(, __FLUFFYHEAP_NULLABLE(fluffyheap*), api_mod_debug_hook_fh_new_tail, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams) {
  if (IS_ENABLED(CONFIG_MOD_DEBUG_ALWAYS_ENABLE)) {
    fh_disable_mod(FH_MOD_DEBUG);
    
    // Restore flags if the debug mod was enabled
    if ((prevDebugFlags & FH_MOD_WAS_ENABLED) != 0)
      fh_enable_mod(FH_MOD_DEBUG, prevDebugFlags & (~FH_MOD_WAS_ENABLED));
  }
  
  ci->action = HOOK_CONTINUE;
  return NULL;
}
