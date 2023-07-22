#ifndef _headers_1689987330_FluffyGC_id_generator
#define _headers_1689987330_FluffyGC_id_generator

#include <stdint.h>

// 0 and -1 never be valid ID
// wraps around if necessary but
// practically 64-bit has alot of IDs
// which can take hundreds of years to exhaust
uint64_t id_generator_get();

#endif

