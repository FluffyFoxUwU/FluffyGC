#ifndef _headers_1673868968_FluffyGC_bits
#define _headers_1673868968_FluffyGC_bits

#include <stddef.h>

#define BITS_PER_BYTE 8

#define BITS_PER_LONG_LONG (sizeof(long long) * BITS_PER_BYTE)
#define BITS_PER_LONG (sizeof(long) * BITS_PER_BYTE)

#define BIT_ULL(nr)       (1ULL << (nr))
#define BIT_ULL_MASK(nr)  (1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)  ((nr) / BITS_PER_LONG_LONG)

#define BIT_UL(nr)      (1UL << (nr))
#define BIT_MASK(nr)    (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)    ((nr) / BITS_PER_LONG)

#define BIT(x)  (1UL << (x))

#endif

