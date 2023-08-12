#ifndef _headers_1671187809_FluffyLauncher_panic
#define _headers_1671187809_FluffyLauncher_panic

#include <stdarg.h>

#include "util/util.h"

// Panic for use when continuing always result in crash

[[noreturn]]
void _panic(const char* fmt, ...);

[[noreturn]]
void _panic_va(const char* fmt, va_list list);

[[noreturn]]
void _hard_panic(const char* fmt, ...);

[[noreturn]]
void _hard_panic_va(const char* fmt, va_list list);

#define panic_fmt(fmt) __FILE__ ":" stringify(__LINE__) ": " fmt
#define panic(fmt, ...) _panic(panic_fmt(fmt) __VA_OPT__(,) __VA_ARGS__) 
#define panic_va(fmt, args) _panic_va(panic_fmt(fmt), args) 

// Hard panic bypass everything that panic normally does
// specifically hard panic directly prints to stderr, skipping
// normal logger path
//
// Hard panic must be used sparingly, only ever
// used if normal panic may cause deadlocks on
// certain paths, for example inside circular buffer
// which depends on logger and normal panic logs into
// logger which may be trigger same cause and hang forever.
#define hard_panic(fmt, ...) _hard_panic(panic_fmt(fmt) __VA_OPT__(,) __VA_ARGS__) 
#define hard_panic_va(fmt, args) _hard_panic_va(panic_fmt(fmt), args)

#endif

