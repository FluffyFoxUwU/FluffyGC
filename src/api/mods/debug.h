#ifndef _headers_1689412335_FluffyGC_debug
#define _headers_1689412335_FluffyGC_debug

#include "mods/dma.h"

struct api_mod_state;

int api_mod_debug_check_flags(uint32_t flags);
int api_mod_debug_init(struct api_mod_state* self);
void api_mod_debug_cleanup(struct api_mod_state* self);

#endif

