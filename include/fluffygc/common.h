#ifndef header_1658554059_716ccf65_7e58_4ac1_b78b_7a8a8ab969a1_common_h
#define header_1658554059_716ccf65_7e58_4ac1_b78b_7a8a8ab969a1_common_h

// Common stuffs

#ifdef __GNUC__
# define FLUFFYGC_ATTRIBUTE(x) __attribute__(x)
#else
# define FLUFFYGC_ATTRIBUTE(x)
#endif

#define FLUFFYGC_EXPORT FLUFFYGC_ATTRIBUTE((visibility ("default")))

#endif

