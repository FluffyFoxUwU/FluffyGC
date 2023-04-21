#ifndef header_1656164800_compiler_config_h
#define header_1656164800_compiler_config_h

////////////////////////////////////////
// Compiler config                    //
////////////////////////////////////////

#ifdef __has_feature
# if __has_feature(address_sanitizer)
#  define CONFIG_BUILD_ASAN 1
# endif
# if __has_feature(thread_sanitizer)
#  define CONFIG_BUILD_TSAN 1
# endif
# if __has_feature(undefined_behavior_sanitizer)
#  define CONFIG_BUILD_UBSAN 1
# endif
# if __has_feature(memory_sanitizer)
#  define CONFIG_BUILD_MSAN 1
# endif
#endif

#endif

