#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <errno.h>

#include "api/mods/debug/common.h"
#include "attributes.h"
#include "buffer.h"
#include "panic.h"
#include "api/api.h"
#include "api/mods/debug/debug.h"
#include "config.h"
#include "context.h"
#include "FluffyHeap.h"
#include "managed_heap.h"
#include "panic.h"

// Debug mode

int debug_check_flags(uint32_t flags) {
  return 0;
}

bool debug_can_do_check() {
  return managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].data.debugMod.verbosity > MOD_DEBUG_SILENT;
}

bool debug_can_do_api_verbose() {
  return managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].data.debugMod.verbosity >= MOD_DEBUG_ULTRA_VERBOSE;
}

int debug_init(struct api_mod_state* self) {
  self->data.debugMod.panicOnWarn = IS_ENABLED(CONFIG_MOD_DEBUG_PANIC);
  self->data.debugMod.verbosity = MOD_DEBUG_ULTRA_VERBOSE;
  return 0;
}

void debug_cleanup(struct api_mod_state* self) {
  return;
}

ATTRIBUTE_PRINTF(1, 2)
static void writeDebugOutputf(const char* fmt, ...) {
  va_list args;
  va_start(args);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void debug_info(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  
  static thread_local char buffer[512 * 1024];
  vsnprintf(buffer, sizeof(buffer), msg, args);
  
  writeDebugOutputf("[DEBUG] [INFO] %s\n", buffer);
  va_end(args);
}

void debug_warn(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  
  static thread_local char buffer[512 * 1024];
  vsnprintf(buffer, sizeof(buffer), msg, args);
  
  if (managed_heap_current->api.state->modManager.modStates[FH_MOD_DEBUG].data.debugMod.panicOnWarn)
    panic("Ouch -w-: %s", buffer);
  else
    writeDebugOutputf("[DEBUG] [WARN] %s\n", buffer);
  va_end(args);
}

static int vformatAndAppend(enum debug_argument_type entryType, buffer_t* buffer, va_list args) {
  int ret = 0;
  switch (entryType) {
    case DEBUG_ARG_FH_ARRAY:
      if ((ret = buffer_append(buffer, debug_get_unique_name(API_EXTERN(API_INTERN(va_arg(args, fh_array*)))))) < 0)
        goto not_enough_memory;
      break;
    case DEBUG_ARG_SIZE_T:
      if ((ret = buffer_appendf(buffer, "%zu", va_arg(args, size_t))) < 0)
        goto not_enough_memory;
      break;
    case DEBUG_ARG_SSIZE_T:
      if ((ret = buffer_appendf(buffer, "%zd", va_arg(args, ssize_t))) < 0)
        goto not_enough_memory;
      break;
    case DEBUG_ARG_FH_OBJECT:
      if ((ret = buffer_append(buffer, debug_get_unique_name(va_arg(args, fh_object*)))) < 0)
        goto not_enough_memory;
      break;
  }
not_enough_memory:
  return ret;
}

void debug_helper_print_api_call(const char* source, enum debug_argument_type* argTypes, size_t argCount, ...) {
  va_list list;
  va_start(list, argCount);
  buffer_t* buffer = buffer_new();
  int ret = 0;
  if (!buffer) {
    ret = -ENOMEM;
    goto not_enough_memory;
  }
  
  if ((ret = buffer_appendf(buffer, "API call: %s(", source)) < 0)
    goto not_enough_memory;
  
  for (size_t i = 0; i < argCount; i++) {
    if ((ret = vformatAndAppend(argTypes[i], buffer, list)) < 0)
      goto not_enough_memory;
    
    // Next entry is not last so put comma
    if (i + 1 == argCount - 1)
      if ((ret = buffer_append(buffer, ", ")) < 0)
        goto not_enough_memory;
  }
  
  if ((ret = buffer_append(buffer, ")")) < 0)
    goto not_enough_memory;
  
  writeDebugOutputf("[DEBUG] [API call info] %s = ", buffer_string(buffer));
not_enough_memory:
  if (ret < 0)
    writeDebugOutputf("[DEBUG] [API call info] %s: Called! (Insufficient memory to show arguments)\n", source);
  buffer_free(buffer);
  va_end(list);
}

void debug_helper_process_return(const char* source, enum debug_argument_type retType, ...) {
  va_list args;
  va_start(args);
  
  int ret = 0;
  buffer_t* buffer = buffer_new();
  if (!buffer) {
    ret = -ENOMEM;
    goto not_enough_memory;
  }
  
  if ((ret = vformatAndAppend(retType, buffer, args)) < 0)
    goto not_enough_memory;
  writeDebugOutputf("%s\n", buffer_string(buffer));
not_enough_memory:
  if (ret < 0)
    writeDebugOutputf("<Insufficient memory to show return value>\n");
  va_end(args);
  buffer_free(buffer);
}
