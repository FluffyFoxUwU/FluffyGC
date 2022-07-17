#ifndef header_1656164800_compiler_config_h
#define header_1656164800_compiler_config_h

////////////////////////////////////////
// Compiler config                    //
////////////////////////////////////////

#ifdef __GNUC__
# define ATTRIBUTE(x) __attribute__(x)
#else
# define ATTRIBUTE(x)
#endif

#ifdef __has_feature
# if __has_feature(address_sanitizer)
#  define FLUFFYGC_ASAN_ENABLED (1)
# endif
# if __has_feature(thread_sanitizer)
#  define FLUFFYGC_TSAN_ENABLED (1)
# endif
# if __has_feature(undefined_behavior_sanitizer)
#  define FLUFFYGC_UBSAN_ENABLED (1)
# endif
# if __has_feature(memory_sanitizer)
#  define FLUFFYGC_MSAN_ENABLED (1)
# endif
#endif

#ifndef FLUFFYGC_TSAN_ENABLED
# define FLUFFYGC_TSAN_ENABLED (0)
#endif

#ifndef FLUFFYGC_ASAN_ENABLED
# define FLUFFYGC_ASAN_ENABLED (0)
#endif

#ifndef FLUFFYGC_UBSAN_ENABLED
# define FLUFFYGC_UBSAN_ENABLED (0)
#endif

#ifndef FLUFFYGC_MSAN_ENABLED
# define FLUFFYGC_MSAN_ENABLED (0)
#endif

#endif

