#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "api/mods/mods.h"
#include "managed_heap.h"
#include "util/util.h"

// Heap is accessible via global

struct call {
  int (*init)(struct managed_heap* heap);
  void (*cleanup)(struct managed_heap* heap);
};

struct call calls[] = {
  {api_mods_init, api_mods_cleanup}
};

int api_init() {
  int ret = 0;
  
  managed_heap_current->api.state = NULL;
  struct api_state* state = malloc(sizeof(*state));
  *state = (struct api_state) {};
  if (!state)
    return -ENOMEM;
  managed_heap_current->api.state = state;
  
  for (; state->initCounts < ARRAY_SIZE(calls); state->initCounts++)
    if ((ret = calls[state->initCounts].init(managed_heap_current)) < 0)
      goto failure;
failure:
  return ret;
}

void api_cleanup() {
  if (!managed_heap_current->api.state)
    return;
  
  struct api_state* state = managed_heap_current->api.state;
  for (; state->initCounts > 0; state->initCounts--)
    calls[state->initCounts - 1].cleanup(managed_heap_current);
  free(managed_heap_current->api.state);
}
