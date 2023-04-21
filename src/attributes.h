#ifndef _headers_1682052855_FluffyGC_attributes
#define _headers_1682052855_FluffyGC_attributes

#ifdef __GNUC__
# define ATTRIBUTE(x) __attribute__(x)
#else
# define ATTRIBUTE(x)
#endif

#define ATTRIBUTE_PRINTF(fmtOffset, vaStart) ATTRIBUTE((format(printf, fmtOffset, vaStart)))
#define ATTRIBUTE_SCANF(fmtOffset, vaStart) ATTRIBUTE((format(scanf, fmtOffset, vaStart)))

// Properly tag function exported functions with this to ensure LTO not removing them
#define ATTRIBUTE_USED() ATTRIBUTE((used))

#endif

