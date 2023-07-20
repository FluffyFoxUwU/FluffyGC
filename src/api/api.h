#ifndef _headers_1687155697_FluffyGC_api
#define _headers_1687155697_FluffyGC_api

#include "FluffyHeap.h"
#include "api/mods/mods.h"
#include "hook/hook.h"

struct api_mod_state;

int api_init();
void api_cleanup();

#define API_INTERN(c) _Generic ((c), \
  fluffyheap*: (struct managed_heap*) (c), \
  fh_object*: (struct root_ref*) (c), \
  fh_context*: (struct context*) (c), \
  fh_descriptor*: (struct descriptor*) (c), \
  fh_array*: (struct root_ref*) (c) \
)

#define API_EXTERN(c) _Generic ((c), \
  struct managed_heap*: (fluffyheap*) (c), \
  struct root_ref*: (fh_object*) (c), \
  struct context*: (fh_context*) (c), \
  struct descriptor*: (fh_descriptor*) (c) \
)

struct api_state {
  struct api_mod_state modStates[FH_MOD_COUNT];
  int initCounts;
};

#define API_FUNCTION_DEFINE(...) \
  __FLUFFYHEAP_EXPORT HOOK_TARGET(__VA_ARGS__)
#define API_FUNCTION_DEFINE_VOID(...) \
  __FLUFFYHEAP_EXPORT HOOK_TARGET_VOID(__VA_ARGS__)

#endif

