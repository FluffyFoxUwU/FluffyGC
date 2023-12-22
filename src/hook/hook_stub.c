#include <errno.h>

#include "hook.h"
#include "panic.h"
#include "macros.h"

// Stub when hook disabled

int hook_init() {
  return -ENOSYS;
}

void hook_cleanup() {}

bool hook__run_head(struct hook_internal_state* state, void* ret, ...) {
  UNUSED(state);
  UNUSED(ret);
  panic("Hooking infrastructure is disabled!");
}

bool hook__run_tail(struct hook_internal_state* state, void* ret, ...) {
  UNUSED(state);
  UNUSED(ret);
  panic("Hooking infrastructure is disabled!");
}

bool hook__run_invoke(struct hook_internal_state* state, void* ret, ...) {
  UNUSED(state);
  UNUSED(ret);
  panic("Hooking infrastructure is disabled!");
}


int hook__register(void* target, enum hook_location location, hook_func func) {
  UNUSED(target);
  UNUSED(location);
  UNUSED(func);
  return -ENOSYS;
}

void hook__unregister(void* target, enum hook_location location, hook_func func) {
  UNUSED(target);
  UNUSED(location);
  UNUSED(func);
}

void hook__enter_function(void* func, struct hook_internal_state* ci) {
  UNUSED(func);
  UNUSED(ci);
}

void hook__exit_function(struct hook_internal_state* ci) {
  UNUSED(ci);
}
