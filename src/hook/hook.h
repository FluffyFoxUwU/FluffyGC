#ifndef _headers_1689759444_FluffyGC_hook
#define _headers_1689759444_FluffyGC_hook

#include <stdarg.h>
#include <stddef.h>

#include "config.h"
#include "rcu/rcu.h"
#include "util/macro_magics.h"
#include "util/util.h"

// Hook into a opt-ed in functions

enum hook_action {
  // Continue with other hook function
  // invoke hook can return to override
  HOOK_CONTINUE,
  
  // Return with a value
  HOOK_RETURN
};

enum hook_location {
  HOOK_HEAD,
  HOOK_TAIL,
  HOOK_INVOKE,
  HOOK_COUNT
};

struct hook_call_info {
  const char* name;
  // func points to the faked function not the real one
  void* func;
  void* returnValue;
  enum hook_action action;
  enum hook_location location;
};

struct hook_internal_state {
  const char* funcName;
  struct rcu_head* targetEntryRcuTemp;
  struct target_entry_rcu* targetRcu;
  void* func;
};

typedef enum hook_action (*hook_func)(struct hook_call_info*, va_list args);

#define ____HOOK_TARGET_IGNORE(a, ...)
#define ____HOOK_TARGET_PASSTHROUGHA(a, ...) a
#define ____HOOK_TARGET_PASSTHROUGHB(a, ...) a __VA_OPT__(,)
#define ____HOOK_MAKE_ARGS(...) MACRO_FOR_EACH_STRIDE2(____HOOK_TARGET_PASSTHROUGHA, ____HOOK_TARGET_PASSTHROUGHB, __VA_ARGS__)
#define ____HOOK_DECLARE(ret, name, ...) ret name(____HOOK_MAKE_ARGS(__VA_ARGS__))
#define ____HOOK_TO_REAL(name) ____hook_func_ ## name ## _real
#define ____HOOK_TO_HANDLER(name) ____hook_handler_ ## name
#define ____HOOK_TO_TARGET_TYPE(name) ____hook_target_ ## name ## _type
#define ____HOOK_PASS_ARGUMENTS(...) MACRO_FOR_EACH_STRIDE2(____HOOK_TARGET_IGNORE, ____HOOK_TARGET_PASSTHROUGHB, __VA_ARGS__)

int hook_init();

bool hook__run_head(struct hook_internal_state* state, void* ret, ...);
bool hook__run_tail(struct hook_internal_state* state, void* ret, ...);

// Invoke basicly replaces the function if there
// defined hooks
bool hook__run_invoke(struct hook_internal_state* state, void* ret, ...);

int hook__register(void* target, enum hook_location location, hook_func func);
void hook__unregister(void* target, enum hook_location location, hook_func func);

void hook__enter_function(void* func, struct hook_internal_state* ci);
void hook__exit_function(struct hook_internal_state* ci);

#ifdef __GNUC__
#   define hook_register(target, location, func) ({ \
  ____HOOK_TO_TARGET_TYPE(func) a = target; \
  a; \
  hook__register(target, location, ____HOOK_TO_HANDLER(func)); \
})
#   define hook_unregister(target, location, func) do { \
  ____HOOK_TO_TARGET_TYPE(func) a = target; \
  a; \
  hook__unregister(target, location, ____HOOK_TO_HANDLER(func)); \
} while (0);
#else
#define hook_register(target, location, func) hook__register(target, location, ____HOOK_TO_HANDLER(func))
#define hook_unregister(target, location, func) hook__unregister(target, location, ____HOOK_TO_HANDLER(func))
#endif

#define ____HOOK_COMMA ,
#define ____HOOK_PROCESS_ARGUMENT(a, ...) va_arg(args, a) MACRO_EXPAND_IF_NOT1(MACRO_NARG(__VA_ARGS__), ____HOOK_COMMA)
#define HOOK_FUNCTION_DECLARE(spec, ret, name, ...) \
  typedef ret (*____HOOK_TO_TARGET_TYPE(name))(____HOOK_MAKE_ARGS(__VA_ARGS__)); \
  spec ____HOOK_DECLARE(void, name, ret*, returnLocation, NULLABLE(struct hook_call_info*), ci __VA_OPT__(,) __VA_ARGS__); \
  spec enum hook_action ____HOOK_TO_HANDLER(name)(NONNULLABLE(struct hook_call_info*) ci, va_list args); \
  spec ____HOOK_DECLARE(void, name, ret*, returnLocation, NULLABLE(struct hook_call_info*), ci __VA_OPT__(,) __VA_ARGS__)

#define HOOK_FUNCTION(spec, ret, name, ...) \
  typedef ret (*____HOOK_TO_TARGET_TYPE(name))(____HOOK_MAKE_ARGS(__VA_ARGS__)); \
  spec ____HOOK_DECLARE(void, name, ret*, returnLocation, struct hook_call_info*, ci __VA_OPT__(,) __VA_ARGS__); \
  spec enum hook_action ____HOOK_TO_HANDLER(name)(struct hook_call_info* ci, va_list args) { \
    ret retVal; \
    name(&retVal, ci __VA_OPT__(,) MACRO_FOR_EACH_STRIDE2(____HOOK_PROCESS_ARGUMENT, ____HOOK_TARGET_IGNORE, __VA_ARGS__)); \
    if (ci->action == HOOK_RETURN) \
      *(ret*) ci->returnValue = retVal; \
    return ci->action; \
  } \
  spec ____HOOK_DECLARE(void, name, ret*, returnLocation, struct hook_call_info*, ci __VA_OPT__(,) __VA_ARGS__)

#if IS_ENABLED(CONFIG_HOOK)
#define HOOK_TARGET(ret, name, ...) \
  /* Declare faked foo */ \
  ____HOOK_DECLARE(ret, name __VA_OPT__(,) __VA_ARGS__); \
  /* Declare real foo */ \
  static inline ____HOOK_DECLARE(ret, ____HOOK_TO_REAL(name) __VA_OPT__(,) __VA_ARGS__); \
  /* Define faked foo */ \
  ____HOOK_DECLARE(ret, name, __VA_ARGS__) { \
    ret retVal; \
    ret realRetVal; \
    struct hook_internal_state internal = { \
      .funcName = __func__ \
    }; \
    hook__enter_function(name, &internal); \
    if (hook__run_head(&internal, &retVal __VA_OPT__(,) ____HOOK_PASS_ARGUMENTS(__VA_ARGS__))) { \
      realRetVal = retVal; \
      goto quit_function; \
    } \
    if (hook__run_invoke(&internal, &retVal __VA_OPT__(,) ____HOOK_PASS_ARGUMENTS(__VA_ARGS__))) \
      realRetVal = retVal; \
    else \
      realRetVal = ____HOOK_TO_REAL(name)(____HOOK_PASS_ARGUMENTS(__VA_ARGS__)); \
quit_function: \
    if (hook__run_tail(&internal, &retVal __VA_OPT__(,) ____HOOK_PASS_ARGUMENTS(__VA_ARGS__))) { \
      realRetVal = retVal; \
      goto quit_function; \
    } \
    hook__exit_function(&internal); \
    return realRetVal; \
  } \
  /* Define real foo */ \
  static inline ____HOOK_DECLARE(ret, ____HOOK_TO_REAL(name) __VA_OPT__(,) __VA_ARGS__)
#define HOOK_TARGET_VOID(name, ...) \
  /* Declare faked foo */ \
  ____HOOK_DECLARE(void, name __VA_OPT__(,) __VA_ARGS__); \
  /* Declare real foo */ \
  static inline ____HOOK_DECLARE(void, ____HOOK_TO_REAL(name) __VA_OPT__(,) __VA_ARGS__); \
  /* Define faked foo */ \
  ____HOOK_DECLARE(void, name, __VA_ARGS__) { \
    struct hook_internal_state internal = { \
      .funcName = __func__ \
    }; \
    hook__enter_function(name, &internal); \
    if (hook__run_head(&internal, NULL __VA_OPT__(,) ____HOOK_PASS_ARGUMENTS(__VA_ARGS__))) \
      goto quit_function; \
    if (!hook__run_invoke(&internal, NULL __VA_OPT__(,) ____HOOK_PASS_ARGUMENTS(__VA_ARGS__))) \
      ____HOOK_TO_REAL(name)(____HOOK_PASS_ARGUMENTS(__VA_ARGS__)); \
quit_function: \
    hook__run_tail(&internal, NULL __VA_OPT__(,) ____HOOK_PASS_ARGUMENTS(__VA_ARGS__)); \
    hook__exit_function(&internal); \
    return; \
  } \
  /* Define real foo */ \
  static inline ____HOOK_DECLARE(void, ____HOOK_TO_REAL(name) __VA_OPT__(,) __VA_ARGS__)
#else
#define HOOK_TARGET(ret, name, ...) ____HOOK_DECLARE(ret, name, __VA_ARGS__)
#define HOOK_TARGET_VOID(ret, name, ...) ____HOOK_DECLARE(ret, name, __VA_ARGS__)
#endif

/*

HOOK_TARGET(int, foo, int, a, int, b) {
  return a * b;
}

// First method
static inline int realFoo(int a, int b) {
  return a * b;
}

int foo(int a, int b) {
  int ret;
  struct hook_call_info ci = {
    .func = foo,
    .returnValue = &ret;
  };
  if (doHeadHook(ci, a, b) == HOOK_RETURN)
    return ret;
  ret = realFoo(a, b);
  doTailHook(ci, a, b);
  return ret;
}

*/

#endif

