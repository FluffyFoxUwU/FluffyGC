#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "bug.h"
#include "hook.h"
#include "concurrency/mutex.h"
#include "panic.h"
#include "rcu/rcu.h"
#include "rcu/rcu_generic.h"
#include "rcu/rcu_generic_type.h"
#include "rcu/rcu_vec.h"
#include "util/util.h"
#include "vec.h"

RCU_GENERIC_TYPE_DEFINE_TYPE(target_entry_rcu, struct target_entry);
RCU_VEC_DEFINE_TYPE(target_entry_list_rcu, struct target_entry_rcu*);

struct target_entry {
  void* target;
  vec_t(hook_func) hooksLocations[HOOK_COUNT];
};

static void targetEntryCleanup(void* _data) {
  struct target_entry* data = _data;
  for (int i = 0; i < HOOK_COUNT; i++)
    vec_deinit(&data->hooksLocations[i]);
}

static int targetEntryCopy(const void* _src, void* _dest) {
  int ret = 0;
  const struct target_entry* src = _src;
  struct target_entry* dest = _dest;
  *dest = *src;
  
  // Initialize
  for (int i = 0; i < HOOK_COUNT; i++)
    vec_init(&dest->hooksLocations[i]);
  
  // Reserve
  for (int i = 0; i < HOOK_COUNT; i++)
    if (vec_reserve(&dest->hooksLocations[i], src->hooksLocations[i].length) < 0)
      goto failure;
  
  // Copy
  for (int i = 0; i < HOOK_COUNT; i++) {
    for (int j = 0; j < src->hooksLocations[i].length; j++)
      dest->hooksLocations[i].data[j] = src->hooksLocations[i].data[j];
    dest->hooksLocations[i].length = src->hooksLocations[i].length;
  }

failure:
  if (ret < 0)
    for (int i = 0; i < HOOK_COUNT; i++)
      vec_deinit(&dest->hooksLocations[i]);
  return ret;
}

static struct rcu_generic_ops targetEntryOps = {
  .cleanup = targetEntryCleanup,
  .copy = targetEntryCopy
};

// Lock this whenever trying to change the target entry
// list and their contents
static struct mutex globalHookLock = MUTEX_INITIALIZER;

// Control so that init start once
static struct mutex startupLock = MUTEX_INITIALIZER;

static bool hasInitialized = false;
static struct target_entry_list_rcu targetsList;

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

static int addHookTarget(struct target_entry_list_rcu_writeable_container* list, void* target) {
  int ret = 0;
  struct target_entry_rcu_writeable_container targetEntryContainer = rcu_generic_type_alloc_for(rcu_vec_element_type(targetsList.dataType));
  if (!targetEntryContainer.data) {
    ret = -ENOMEM;
    goto preparation_failure;
  }
  targetEntryContainer.data->target = target;
  for (int i = 0; i < HOOK_COUNT; i++)
    vec_init(&targetEntryContainer.data->hooksLocations[i]);
  
  struct target_entry_rcu* rcuPointer = malloc(sizeof(*rcuPointer));
  if (!rcuPointer) {
    ret = -ENOMEM;
    goto preparation_failure;
  }
  
  if ((ret = rcu_generic_type_init(rcuPointer, &targetEntryOps)) < 0)
    goto preparation_failure;
  
  rcu_generic_type_write(rcuPointer, &targetEntryContainer);
  
  mutex_lock(&globalHookLock);
  struct target_entry_list_rcu_writeable_container listCopy;
  
  if (list) {
    listCopy = *list;
  } else {
    listCopy = rcu_generic_type_copy(&targetsList);
    if (!listCopy.data)
      goto insert_failure;
  }
  
  if (vec_push(&listCopy.data->array, rcuPointer) < 0) {
    ret = -ENOMEM;
    goto insert_failure;
  }
  
  // The list copy was created by this function
  // so write back the result
  if (!list)
    rcu_generic_type_write(&targetsList, &listCopy);
insert_failure:
  if (ret < 0 && !list)
    rcu_generic_dealloc_container(listCopy.container);
  
  mutex_unlock(&globalHookLock);
preparation_failure:
  return ret;
}

// RCU read lock on the list must be acquired by the reader
static struct target_entry_rcu* getTarget(void* func) {
  struct rcu_head* const targetListRcuTmp = rcu_read_lock(rcu_generic_type_get_rcu(&targetsList));
  struct target_entry_list_rcu_readonly_container list = rcu_generic_type_get(&targetsList);
  
  // TODO: Turn this into bsearch
  struct target_entry_rcu* result = NULL;
  struct target_entry_rcu* current = NULL;
  int i = 0;
  vec_foreach(&list.data->array, current, i) {
    struct rcu_head* const entryRcuTmp = rcu_read_lock(rcu_generic_type_get_rcu(current));
    struct target_entry_rcu_readonly_container currentContainer = rcu_generic_type_get(current);
    
    if (currentContainer.data->target == func) {
      result = current;
      rcu_read_unlock(rcu_generic_type_get_rcu(current), entryRcuTmp);
      goto found_result;
    }
    
    rcu_read_unlock(rcu_generic_type_get_rcu(current), entryRcuTmp);
  }
  
found_result:
  rcu_read_unlock(rcu_generic_type_get_rcu(&targetsList), targetListRcuTmp);
  return result;
}

static int hookRegister(void* target, enum hook_location location, hook_func func) {
  struct target_entry_rcu* targetEntry = getTarget(target);
  int ret = 0;
  
  if (!targetEntry) {
    ret = -ENODEV;
    goto target_not_found;
  }
  
  struct rcu_head* const targetEntryRcuTmp = rcu_read_lock(rcu_generic_type_get_rcu(targetEntry));
  struct target_entry_rcu_readonly_container targetEntryReadonly = rcu_generic_type_get(targetEntry);
  
  if (location == HOOK_INVOKE && targetEntryReadonly.data->hooksLocations[location].length == 1) {
    rcu_read_unlock(rcu_generic_type_get_rcu(targetEntry), targetEntryRcuTmp);
    ret = -EINVAL;
    goto invalid_register;
  }
  
  struct target_entry_rcu_writeable_container targetEntryWriteable = rcu_generic_type_copy(targetEntry);
  rcu_read_unlock(rcu_generic_type_get_rcu(targetEntry), targetEntryRcuTmp);
  
  if (!targetEntryWriteable.data) {
    ret = -ENOMEM;
    goto error_registering;
  }
  
  if (location == HOOK_INVOKE && targetEntryWriteable.data->hooksLocations[location].length == 1) {
    ret = -EINVAL;
    goto invalid_register;
  }
  
  if (vec_push(&targetEntryWriteable.data->hooksLocations[location], func) < 0) {
    ret = -ENOMEM;
    goto error_registering;
  }
  
  // Sort for bsearch usage later on
  typeof(targetEntryWriteable.data->hooksLocations[location])* hookArray = &targetEntryWriteable.data->hooksLocations[location];
  qsort(hookArray->data, hookArray->length, sizeof(*hookArray->data), compareHookFunc);
  
  rcu_generic_type_write(targetEntry, &targetEntryWriteable);
error_registering:
  if (ret < 0)
    rcu_generic_dealloc_container(targetEntryWriteable.container);
invalid_register:
target_not_found:
  return ret;
}

enum register_type {
  REGISTER_TARGET,
  REGISTER_HOOK_FUNC,
};
static int registerFunctions(enum register_type registerType, struct target_entry_list_rcu_writeable_container* listWriteAccess) {
  int ret = 0;
# define ADD_HOOK_TARGET(target)  do { \
  if (registerType == REGISTER_TARGET && (ret = addHookTarget(listWriteAccess, target)) < 0) { \
    BUG_ON(ret != -ENOMEM); \
    goto add_target_failure; \
  } \
} while (0)
# define ADD_HOOK_FUNC(target, location, func) do { \
  if (registerType == REGISTER_HOOK_FUNC && (ret = hook_register(target, location, func)) < 0) { \
    BUG_ON(ret != -ENOMEM); \
    goto add_hook_failure; \
  } \
} while (0)

// Do the thing
# include "hook_list.h"

# undef ADD_HOOK_FUNC
# undef ADD_HOOK_TARGET
  UWU_SO_THAT_UNUSED_COMPLAIN_SHUT_UP;
add_target_failure:
add_hook_failure:
  return ret;
}

static void hookCleanup() {
  // Clean up
  struct rcu_head* rcuTmp = rcu_read_lock(rcu_generic_type_get_rcu(&targetsList));
  struct target_entry_list_rcu_readonly_container list = rcu_generic_type_get(&targetsList); 
  if (!list.data)
    goto target_list_not_initialized;
  
  int i = 0;
  struct target_entry_rcu* entry;
  vec_foreach(&list.data->array, entry, i)
    rcu_generic_type_write(entry, NULL);

target_list_not_initialized:
  rcu_read_unlock(rcu_generic_type_get_rcu(&targetsList), rcuTmp);
  rcu_generic_type_write(&targetsList, NULL);
}

int hook_init() {
  mutex_lock(&startupLock);
  int ret = 0;
  if (hasInitialized)
    goto already_initialized;
  
  if ((ret = rcu_vec_init_rcu(&targetsList)) < 0)
    goto failed_initializing_target_list;
  
  struct target_entry_list_rcu_writeable_container list = rcu_generic_type_alloc_for(&targetsList);
  if (!list.data) {
    ret = -ENOMEM;
    goto failed_initializing_target_list;
  }
  rcu_vec_init(&list);
  
  if ((ret = registerFunctions(REGISTER_TARGET, &list)) < 0)
    goto registering_target_failed;
  rcu_generic_type_write(&targetsList, &list);
  
  // Sort so bsearch works
  typeof(list.data->array)* vecList = &list.data->array;
  qsort(vecList->data, vecList->length, sizeof(*vecList->data), compareTarget);
  
  list = rcu_generic_type_copy(&targetsList);
  if (!list.data) {
    ret = -ENOMEM;
    goto failed_copy_target_list;
  }
  
  if ((ret = registerFunctions(REGISTER_HOOK_FUNC, &list)) < 0)
    goto registering_hook_failed;
  
  rcu_generic_type_write(&targetsList, &list);
  hasInitialized = true;
registering_hook_failed:
registering_target_failed:
  if (ret < 0) {
    rcu_generic_dealloc_container(list.container);
    hookCleanup();
  }
failed_copy_target_list:
failed_initializing_target_list:
already_initialized:
  mutex_unlock(&startupLock);
  return ret;
}

static hook_func** findHookFunc(const struct target_entry* target, enum hook_location location, hook_func func) {
  if (target->hooksLocations[location].data)
    return bsearch(&func, target->hooksLocations[location].data, target->hooksLocations[location].length, sizeof(*target->hooksLocations[location].data), compareHookFunc);
  return NULL;
}

// Read lock on target entry RCU must be held
static bool runHandlers(enum hook_location location, struct hook_internal_state* state, void* ret, va_list args) {
  bool doReturn = false;
  
  struct target_entry_rcu* targetRcu = state->targetRcu;
  struct target_entry_rcu_readonly_container target = rcu_generic_type_get(targetRcu);
  
  // Run each handler
  hook_func hookFunc;
  int i = 0;
  vec_foreach(&target.data->hooksLocations[location], hookFunc, i) {
    struct hook_call_info ci = {
      .func = state->func,
      .action = HOOK_CONTINUE,
      .returnValue = ret,
      .location = location,
      .name = state->funcName
    };
    
    if (hookFunc(&ci, args) == HOOK_RETURN) {
      doReturn = true;
      goto quit;
    };
  }
quit:
  return doReturn;
}

bool hook__run_head(struct hook_internal_state* state, void* ret, ...) {
  va_list args;
  va_start(args, ret);
  bool res = runHandlers(HOOK_HEAD, state, ret, args);
  va_end(args);
  return res;
}

bool hook__run_tail(struct hook_internal_state* state, void* ret, ...) {
  va_list args;
  va_start(args, ret);
  bool res = runHandlers(HOOK_TAIL, state, ret, args);
  va_end(args);
  return res;
}

// Invoke basicly replaces the function if there
// defined hooks
bool hook__run_invoke(struct hook_internal_state* state, void* ret, ...) {
  va_list args;
  va_start(args, ret);
  bool res = runHandlers(HOOK_INVOKE, state, ret, args);
  va_end(args);
  return res;
}

void hook__enter_function(void* func, struct hook_internal_state* state) {
  state->func = func;
  struct target_entry_rcu* targetRcu = getTarget(func);
  if (!targetRcu)
    panic("Target %p not found", func);
  state->targetRcu = targetRcu;
  state->targetEntryRcuTemp = rcu_read_lock(rcu_generic_type_get_rcu(targetRcu));
}

void hook__exit_function(struct hook_internal_state* state) {
  rcu_read_unlock(rcu_generic_type_get_rcu(state->targetRcu), state->targetEntryRcuTemp);
}

int hook__register(void* target, enum hook_location location, hook_func func) {
  return hookRegister(target, location, func);
}

void hook__unregister(void* target, enum hook_location location, hook_func func) {
  struct target_entry_rcu* targetEntryRcu = getTarget(target);
  BUG_ON(!targetEntryRcu);
  
  struct rcu_head* rcuTmp = rcu_read_lock(rcu_generic_type_get_rcu(targetEntryRcu));
  struct target_entry_rcu_readonly_container targetEntry = rcu_generic_type_get(targetEntryRcu);
  hook_func** hookFuncLocation = findHookFunc(targetEntry.data, location, func);
  if (!hookFuncLocation) { 
    rcu_read_unlock(rcu_generic_type_get_rcu(targetEntryRcu), rcuTmp);
    return;
  }
  size_t hookIndex = indexof(targetEntry.data->hooksLocations[location].data, hookFuncLocation);
  
  struct target_entry_rcu_writeable_container targetEntryCopy = rcu_generic_type_copy(targetEntryRcu);
  if (!targetEntryCopy.data)
    panic("TODO: Find right error handling for hook_unregister");
  rcu_read_unlock(rcu_generic_type_get_rcu(targetEntryRcu), rcuTmp);
  
  typeof(targetEntryCopy.data->hooksLocations[location])* hookArray = &targetEntryCopy.data->hooksLocations[location];
  vec_splice(hookArray, hookIndex, 1);
  
  // Sort for bsearch usage later on
  qsort(hookArray->data, hookArray->length, sizeof(*hookArray->data), compareHookFunc);
  rcu_generic_type_write(targetEntryRcu, &targetEntryCopy);
}
