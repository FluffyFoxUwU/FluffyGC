#ifndef _headers_1673868876_FluffyGC_bitops
#define _headers_1673868876_FluffyGC_bitops

#include <stdint.h>
#include <stdbool.h>

#include "bits.h"
#include "util/util.h"

#define BITS_PER_TYPE(type)  (sizeof(type) * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr)    DIV_ROUND_UP(nr, BITS_PER_TYPE(long))
#define BITS_TO_U64(nr)      DIV_ROUND_UP(nr, BITS_PER_TYPE(uint64_t))
#define BITS_TO_U32(nr)      DIV_ROUND_UP(nr, BITS_PER_TYPE(uint32_t))
#define BITS_TO_BYTES(nr)    DIV_ROUND_UP(nr, BITS_PER_TYPE(char))

void setbit(long nr, volatile unsigned long* addr);
void clearbit(long nr, volatile unsigned long* addr);
bool testbit(long nr, volatile unsigned long* addr);

// -1 if not found
// Count is bits to check
int findbit(bool state, volatile unsigned long* addr, unsigned int count);

#endif

