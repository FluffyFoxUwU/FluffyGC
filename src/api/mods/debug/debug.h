#ifndef _headers_1689412335_FluffyGC_debug
#define _headers_1689412335_FluffyGC_debug

#include <stdint.h>
#include <stddef.h>

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

enum debug_argument_type {
  DEBUG_ARG_FH_ARRAY,
  DEBUG_ARG_FH_OBJECT,
  DEBUG_ARG_SIZE_T,
  DEBUG_ARG_SSIZE_T,
};

void debug_helper_print_api_call(const char* source, enum debug_argument_type* argTypes, size_t argCount, ...);
void debug_helper_process_return(const char* source, enum debug_argument_type type, ...);

# define ____debug_try_print_api_call_entry(x, ...) \
  _Generic(typeof(x), \
    size_t: DEBUG_ARG_SIZE_T, \
    ssize_t: DEBUG_ARG_SSIZE_T, \
    fh_array*: DEBUG_ARG_FH_ARRAY, \
    fh_object*: DEBUG_ARG_FH_OBJECT \
  ) __VA_OPT__(,)
# define debug_try_print_api_call(...) \
//do { \
//  static enum debug_argument_type args[] = {MACRO_FOR_EACH(____debug_try_print_api_call_entry, __VA_ARGS__)}; \
//  debug_helper_print_api_call(__FUNCTION__, args, ARRAY_SIZE(args), __VA_ARGS__); \
//} while (0)

#define debug_try_print_api_return_value(v) \
//  debug_helper_process_return(__FUNCTION__, ____debug_try_print_api_call_entry((v)), (v))

#endif

