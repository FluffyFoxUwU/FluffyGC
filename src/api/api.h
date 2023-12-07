#ifndef _headers_1687155697_FluffyGC_api
#define _headers_1687155697_FluffyGC_api

#include "FluffyHeap.h"
#include "api/mods/mods.h"
#include "api/mods/debug/helper.h"

struct api_mod_state;

int api_init();
void api_cleanup();

const char* api_fh_gc_hint_tostring(fh_gc_hint hint);

#define API_INTERN(c) _Generic ((c), \
  fluffyheap*: (struct managed_heap*) (c), \
  fh_object*: (struct root_ref*) (c), \
  fh_context*: (struct context*) (c), \
  fh_descriptor*: (struct descriptor*) (c), \
  fh_array*: (struct root_ref*) (c), \
  fh_object_type: *(enum object_type*) (&(c)), \
  fh_reference_strength: *(enum reference_strength*) (&(c)) \
)

#define API_EXTERN(c) _Generic ((c), \
  struct managed_heap*: (fluffyheap*) (c), \
  struct root_ref*: (fh_object*) (c), \
  struct context*: (fh_context*) (c), \
  struct descriptor*: (fh_descriptor*) (c) \
)

struct api_state {
  struct {
    struct api_mod_state modStates[FH_MOD_COUNT];
    
    // Used to remember how many mods initialized
    int modIteratedCountDuringInit;
  } modManager;
  
  // Used to remember how many part of API initialized
  int initCounts;
};

#define _____API_IGNORE(a, ...) 
#define _____API_PASSTHROUGHA(a, ...) a
#define _____API_PASSTHROUGHB(a, ...) a __VA_OPT__(,)
#define _____API_PASSTHROUGHC(a, ...) typeof(a) __VA_OPT__(,)
#define _____API_MAKE_ARGS(...) MACRO_FOR_EACH_STRIDE2(_____API_PASSTHROUGHA, _____API_PASSTHROUGHB, __VA_ARGS__)

#define API_FUNCTION_DEFINE(ret, name, ...) \
  DEBUG_API_TRACEPOINT(__FLUFFYHEAP_EXPORT, ret, name __VA_OPT__(,) __VA_ARGS__)
#define API_FUNCTION_DEFINE_VOID(name, ...) \
  DEBUG_API_TRACEPOINT_VOID(__FLUFFYHEAP_EXPORT, name __VA_OPT__(,) __VA_ARGS__)

#endif

