#ifndef _headers_1691808714_FluffyGC_helper
#define _headers_1691808714_FluffyGC_helper

#include <stddef.h>
#include <stddef.h>

#include "FluffyHeap.h"
#include "hook/hook.h"
#include "util/macro_magics.h"

enum debug_argument_type {
  DEBUG_TYPE_FH_ARRAY,
  DEBUG_TYPE_FH_OBJECT,
  DEBUG_TYPE_FH_PARAM,
  DEBUG_TYPE_FLUFFYHEAP,
  DEBUG_TYPE_SIZE_T,
  DEBUG_TYPE_SSIZE_T,
  DEBUG_TYPE_VOID
};

void debug_helper_print_api_call(const char* source, enum debug_argument_type* argTypes, size_t argCount, ...);
void debug_helper_process_return(const char* source, enum debug_argument_type type, void* ret);

# define ____debug_try_print_api_call_entry(x, ...) \
  _Generic(x, \
    size_t: DEBUG_TYPE_SIZE_T, \
    ssize_t: DEBUG_TYPE_SSIZE_T, \
    fh_array*: DEBUG_TYPE_FH_ARRAY, \
    fh_object*: DEBUG_TYPE_FH_OBJECT, \
    fh_param*: DEBUG_TYPE_FH_PARAM, \
    fluffyheap*: DEBUG_TYPE_FLUFFYHEAP \
  ) __VA_OPT__(,)

#define ____DEBUG_MOD_API_IGNORE(a, ...) 
#define ____DEBUG_MOD_API_PASSTHROUGHA(a, ...) a
#define ____DEBUG_MOD_API_PASSTHROUGHB(a, ...) a __VA_OPT__(,)
#define ____DEBUG_MOD_API_PASSTHROUGHC(a, ...) typeof(a) __VA_OPT__(,)
#define ____DEBUG_MOD_API_MAKE_ARGS(...) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_PASSTHROUGHA, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__)
#define DEBUG_API_TRACEPOINT(spec, ret2, name, ...) \
  static inline void _____debug_api_tracepoint__real_ ## name(struct hook_call_info* ci __VA_OPT__(,) ____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__)); \
  HOOK_FUNCTION(spec, ret2, name, __VA_ARGS__) { \
    static enum debug_argument_type args[] = {MACRO_FOR_EACH(____debug_try_print_api_call_entry __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__))}; \
    debug_helper_print_api_call(#name, args, ARRAY_SIZE(args) __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__)); \
    _____debug_api_tracepoint__real_ ## name(ci __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__)); \
    debug_helper_process_return(#name, ____debug_try_print_api_call_entry(typeof(ret2)), ci->returnValue); \
  } \
  static inline void _____debug_api_tracepoint__real_ ## name(struct hook_call_info* ci __VA_OPT__(,) ____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__))

#define DEBUG_API_TRACEPOINT_VOID(spec, name, ...) \
  spec void _____debug_api_tracepoint__real_ ## name(struct hook_call_info* ci __VA_OPT__(,) ____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__)); \
  HOOK_FUNCTION(spec, ret2, name, __VA_ARGS__) { \
    static enum debug_argument_type args[] = {MACRO_FOR_EACH(____debug_try_print_api_call_entry __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__))}; \
    debug_helper_print_api_call(#name, args, ARRAY_SIZE(args) __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__)); \
    _____debug_api_tracepoint__real_ ## name(ci __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__)); \
    debug_helper_process_return(#name, DEBUG_TYPE_VOID, NULL); \
  } \
  spec void _____debug_api_tracepoint__real_ ## name(struct hook_call_info* ci __VA_OPT__(,) ____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__))

#endif

