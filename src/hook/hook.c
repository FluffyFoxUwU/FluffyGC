#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "bug.h"
#include "hook.h"
#include "concurrency/mutex.h"
#include "concurrency/rwlock.h"
#include "panic.h"
#include "util/util.h"
#include "vec.h"

struct target_entry {
  void* target;
  vec_t(hook_func) hooksLocations[HOOK_COUNT];
};

// The list and target_entry should be RCU protected
// and this is mutex lock for both
static struct mutex modifyTargetEntryLock = MUTEX_INITIALIZER;

static struct rwlock listLock = RWLOCK_INITIALIZER;
static bool hasInitialized = false;
static vec_t(struct target_entry*) list;

static int compareHookFunc(const void* _a, const void* _b) {
  const hook_func* a = *(const hook_func**) _a;
  const hook_func* b = *(const hook_func**) _b;
  
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  else
    return 0;
}

static int compareTarget(const void* _a, const void* _b) {
  const struct target_entry* a = *(const struct target_entry**) _a;
  const struct target_entry* b = *(const struct target_entry**) _b;
  
  if (a->target > b->target)
    return 1;
  else if (a->target < b->target)
    return -1;
  else
    return 0;
}

static int addHookTarget(void* target) {
  struct target_entry* entry = malloc(sizeof(*entry));
  if (!entry)
    return -ENOMEM;
  
  int ret = 0;
  if (vec_push(&list, entry) < 0) {
    ret = -ENOMEM;
    goto insert_failure;
  }
  
  for (int i = 0; i < HOOK_COUNT; i++)
    vec_init(&entry->hooksLocations[i]);
  entry->target = target;
insert_failure:
  return ret;
}


static struct target_entry* getTarget(void* func) {
  struct target_entry key = {
    .target = func
  };
  struct target_entry* keyIndirect = &key;
  struct target_entry** entry = NULL;
  
  if (list.data)
    entry = bsearch(&keyIndirect, list.data, list.length, sizeof(*list.data), compareTarget);
  
  if (!entry)
    return NULL;
  return *entry;
}

static int hookRegisterNoLock(void* target, enum hook_location location, hook_func func) {
  struct target_entry* targetEntry = getTarget(target);
  int ret = 0;
  
  if (!targetEntry) {
    ret = -ENODEV;
    goto failure;
  }
  
  if (location == HOOK_INVOKE && targetEntry->hooksLocations[location].length == 1) {
    ret = -EINVAL;
    goto failure;
  }
  
  if (vec_push(&targetEntry->hooksLocations[location], func) < 0) {
    ret = -ENOMEM;
    goto failure;
  }
failure:
  return ret;
}

int hook_init() {
  rwlock_rdlock(&listLock);
  int ret = 0;
  if (hasInitialized)
    goto already_initialized;
  
# define ADD_HOOK_TARGET(target) do { \
  if ((ret = addHookTarget(target)) < 0)  \
    goto failure; \
} while (0)
# define ADD_HOOK_FUNC(target, location, func) do { \
  if ((ret = hook_register(target, location, func)) < 0)  \
    goto failure; \
} while (0)
# define _hook_register hookRegisterNoLock

// Do the thing
# include "hook_list.h"

# undef _hook_register
# undef ADD_HOOK_FUNC
# undef ADD_HOOK_TARGET
  UWU_SO_THAT_UNUSED_COMPLAIN_SHUT_UP;
  
  if (list.data)
    qsort(list.data, list.length, sizeof(*list.data), compareTarget);

  hasInitialized = true;
failure:
  if (ret < 0) {
    // Clean up
    struct target_entry* current = NULL;
    int i = 0;
    vec_foreach(&list, current, i) {
      for (int i = 0; i < HOOK_COUNT; i++)
        vec_deinit(&current->hooksLocations[i]);
      free(current);
    }
    vec_deinit(&list);
  }
already_initialized:
  rwlock_unlock(&listLock);
  return ret;
}

static hook_func** findHookFunc(struct target_entry* target, enum hook_location location, hook_func func) {
  if (target->hooksLocations[location].data)
    return bsearch(&func, target->hooksLocations[location].data, target->hooksLocations[location].length, sizeof(*target->hooksLocations[location].data), compareHookFunc);
  return NULL;
}

static bool runHandlers(enum hook_location location, void* func, void* ret, va_list args) {
  rwlock_rdlock(&listLock);
  bool doReturn = false;
  
  struct target_entry* target = getTarget(func);
  if (!target)
    panic("Target %p don't exist", func);
  
  // Run each handler
  hook_func hookFunc;
  int i = 0;
  vec_foreach(&target->hooksLocations[location], hookFunc, i) {
    struct hook_call_info ci = {
      .func = func,
      .action = HOOK_CONTINUE,
      .returnValue = ret,
      .location = location
    };
    
    if (hookFunc(&ci, args) == HOOK_RETURN) {
      doReturn = true;
      goto quit;
    };
  }
quit:
  rwlock_unlock(&listLock);
  return doReturn;
}

bool hook_run_head(void* func, void* ret, ...) {
  va_list args;
  va_start(args, ret);
  bool res = runHandlers(HOOK_HEAD, func, ret, args);
  va_end(args);
  return res;
}

bool hook_run_tail(void* func, void* ret, ...) {
  va_list args;
  va_start(args, ret);
  bool res = runHandlers(HOOK_TAIL, func, ret, args);
  va_end(args);
  return res;
}

// Invoke basicly replaces the function if there
// defined hooks
bool hook_run_invoke(void* func, void* ret, ...) {
  va_list args;
  va_start(args, ret);
  bool res = runHandlers(HOOK_INVOKE, func, ret, args);
  va_end(args);
  return res;
}

int _hook_register(void* target, enum hook_location location, hook_func func) {
  rwlock_wrlock(&listLock);
  int ret = hookRegisterNoLock(target, location, func);
  rwlock_unlock(&listLock);
  return ret;
}

void _hook_unregister(void* target, enum hook_location location, hook_func func) {
  rwlock_wrlock(&listLock);
  struct target_entry* targetEntry = getTarget(target);
  BUG_ON(!targetEntry);
  if (findHookFunc(targetEntry, location, func) == NULL)
    goto failure;
  
  hook_func** hookFuncLocation = findHookFunc(targetEntry, location, func);
  if (!hookFuncLocation)
    goto failure;

  // Copied to take bsearch advantage
  vec_splice(&targetEntry->hooksLocations[location], indexof(targetEntry->hooksLocations[location].data, hookFuncLocation), 1);
failure:
  rwlock_unlock(&listLock);
}
