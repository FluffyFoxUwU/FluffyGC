#ifndef _headers_1691808714_FluffyGC_helper
#define _headers_1691808714_FluffyGC_helper

#include <stddef.h>
#include <stdint.h>

#include "config.h"

#include "hook/hook.h"
#include "util/macro_magics.h"

#include "FluffyHeap.h"
#include "mods/dma.h"

enum debug_argument_type {
  DEBUG_TYPE_FH_ARRAY,
  DEBUG_TYPE_FH_DESCRIPTOR,
  DEBUG_TYPE_FH_OBJECT,
  DEBUG_TYPE_FH_PARAM,
  DEBUG_TYPE_FH_DMA_PTR,
  DEBUG_TYPE_FH_CONTEXT,
  DEBUG_TYPE_FLUFFYHEAP,
  DEBUG_TYPE_FH_DESCRIPTOR_LOADER,
  DEBUG_TYPE_FH_DESCRIPTOR_PARAM,
  DEBUG_TYPE_FH_DESCRIPTOR_PARAM_READONLY,
  DEBUG_TYPE_FH_OBJECT_TYPE,
  DEBUG_TYPE_FH_TYPE_INFO_READONLY,
  DEBUG_TYPE_FH_MOD,
  
  // C and POSIX types
  DEBUG_TYPE_VOID_PTR_READWRITE,
  DEBUG_TYPE_VOID_PTR_READONLY,
  DEBUG_TYPE_TIMESPEC_READONLY,
  DEBUG_TYPE_CSTRING,
  DEBUG_TYPE_BOOL,
  DEBUG_TYPE_INT,
  DEBUG_TYPE_ULONG,
  DEBUG_TYPE_LONG,
  DEBUG_TYPE_ULONGLONG,
  DEBUG_TYPE_LONGLONG,
  DEBUG_TYPE_VOID
};

void debug_helper_print_api_call(const char* source, enum debug_argument_type* argTypes, size_t argCount, ...);
void debug_helper_process_return(const char* source, enum debug_argument_type type, void* ret);

# define ____debug_try_print_api_call_entry(x, ...) \
  _Generic(x, \
    fh_context*: DEBUG_TYPE_FH_CONTEXT, \
    fh_descriptor*: DEBUG_TYPE_FH_DESCRIPTOR, \
    fh_array*: DEBUG_TYPE_FH_ARRAY, \
    fh_object*: DEBUG_TYPE_FH_OBJECT, \
    fh_param*: DEBUG_TYPE_FH_PARAM, \
    fluffyheap*: DEBUG_TYPE_FLUFFYHEAP, \
    fh_dma_ptr*: DEBUG_TYPE_FH_DMA_PTR, \
    fh_descriptor_loader: DEBUG_TYPE_FH_DESCRIPTOR_LOADER, \
    const fh_descriptor_param*: DEBUG_TYPE_FH_DESCRIPTOR_PARAM_READONLY, \
    fh_descriptor_param*: DEBUG_TYPE_FH_DESCRIPTOR_PARAM, \
    fh_object_type: DEBUG_TYPE_FH_OBJECT_TYPE, \
    const fh_type_info*: DEBUG_TYPE_FH_TYPE_INFO_READONLY, \
    fh_mod: DEBUG_TYPE_FH_MOD, \
     \
    unsigned long: DEBUG_TYPE_ULONG, \
    long: DEBUG_TYPE_LONG, \
    unsigned long long: DEBUG_TYPE_ULONGLONG, \
    long long: DEBUG_TYPE_LONGLONG, \
    void*: DEBUG_TYPE_VOID_PTR_READWRITE, \
    const void*: DEBUG_TYPE_VOID_PTR_READONLY, \
    const struct timespec*: DEBUG_TYPE_TIMESPEC_READONLY, \
    const char*: DEBUG_TYPE_CSTRING, \
    bool: DEBUG_TYPE_BOOL, \
    int: DEBUG_TYPE_INT \
  ) __VA_OPT__(,)

#define ____DEBUG_MOD_API_IGNORE(a, ...) 
#define ____DEBUG_MOD_API_PASSTHROUGHA(a, ...) a
#define ____DEBUG_MOD_API_PASSTHROUGHB(a, ...) a __VA_OPT__(,)
#define ____DEBUG_MOD_API_PASSTHROUGHC(a, ...) typeof(a) __VA_OPT__(,)
#define ____DEBUG_MOD_API_MAKE_ARGS(...) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_PASSTHROUGHA, ____DEBUG_MOD_API_PASSTHROUGHB, __VA_ARGS__)

#if IS_ENABLED(CONFIG_MOD_DEBUG)
# define DEBUG_API_TRACEPOINT(spec, ret2, name, ...) \
  static inline ret2 _____debug_api_tracepoint__real_ ## name(____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__)); \
  spec HOOK_TARGET(ret2, name __VA_OPT__(,) __VA_ARGS__) { \
    static enum debug_argument_type args[] = {MACRO_FOR_EACH(____debug_try_print_api_call_entry __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB __VA_OPT__(,) __VA_ARGS__))}; \
    debug_helper_print_api_call(#name, args, ARRAY_SIZE(args) __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB __VA_OPT__(,) __VA_ARGS__)); \
    ret2 returnVal = _____debug_api_tracepoint__real_ ## name(MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB __VA_OPT__(,) __VA_ARGS__)); \
    debug_helper_process_return(#name, ____debug_try_print_api_call_entry(typeof(ret2)), &returnVal); \
    return returnVal; \
  } \
  static inline ret2 _____debug_api_tracepoint__real_ ## name( ____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__))
# define DEBUG_API_TRACEPOINT_VOID(spec, name, ...) \
  static inline void _____debug_api_tracepoint__real_ ## name(____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__)); \
  spec HOOK_TARGET_VOID(name __VA_OPT__(,) __VA_ARGS__) { \
    static enum debug_argument_type args[] = {MACRO_FOR_EACH(____debug_try_print_api_call_entry __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB __VA_OPT__(,) __VA_ARGS__))}; \
    debug_helper_print_api_call(#name, args, ARRAY_SIZE(args) __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB __VA_OPT__(,) __VA_ARGS__)); \
    _____debug_api_tracepoint__real_ ## name(MACRO_FOR_EACH_STRIDE2(____DEBUG_MOD_API_IGNORE, ____DEBUG_MOD_API_PASSTHROUGHB __VA_OPT__(,) __VA_ARGS__)); \
    debug_helper_process_return(#name, DEBUG_TYPE_VOID, NULL); \
  } \
  static inline void _____debug_api_tracepoint__real_ ## name(____DEBUG_MOD_API_MAKE_ARGS(__VA_ARGS__))
#else
# define DEBUG_API_TRACEPOINT_VOID(spec, name, ...) HOOK_FUNCTION_VOID(spec, name, __VA_ARGS__)
# define DEBUG_API_TRACEPOINT(spec, name, ...) HOOK_FUNCTION(spec, name, __VA_ARGS__)
#endif



#endif

