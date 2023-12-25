#ifndef _headers_1689844559_FluffyGC_hook_list
#define _headers_1689844559_FluffyGC_hook_list
// This file only ever included by hook.c

#include "FluffyHeap/FluffyHeap.h"
#include "attributes.h"
#include "hook.h"
#include "macros.h"

#ifndef ADD_HOOK_TARGET
# define STANDALONE
# define ADD_HOOK_TARGET(target) UNUSED(target)
# define ADD_HOOK_FUNC(target, location, func) hook_register(target, location, func)
#endif

#ifndef ADD_HOOK_FUNC_AND_DECLARE
# define ADD_HOOK_FUNC_AND_DECLARE(target, location, ret, func, ...) \
  HOOK_FUNCTION_DECLARE(, ret, func, __VA_ARGS__); \
  ADD_HOOK_FUNC(target, location, func)
#endif

#ifdef STANDALONE
ATTRIBUTE_USED()
void (^uwu)() = ^() {
#endif

//////////////////////////////
// Add new hook target here //
//////////////////////////////

// Example
// ADD_HOOK_TARGET(foo);
// ADD_HOOK_FUNC(foo, );

// Heap management
ADD_HOOK_TARGET(fh_new);
# if IS_ENABLED(CONFIG_MOD_DEBUG_ALWAYS_ENABLE)
  ADD_HOOK_FUNC_AND_DECLARE(fh_new, HOOK_HEAD, __FLUFFYHEAP_NULLABLE(fluffyheap*), debug_hook_fh_new_head, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams);
  ADD_HOOK_FUNC_AND_DECLARE(fh_new, HOOK_TAIL, __FLUFFYHEAP_NULLABLE(fluffyheap*), debug_hook_fh_new_tail, __FLUFFYHEAP_NONNULL(fh_param*), incomingParams);
# endif
ADD_HOOK_TARGET(fh_free);

// Mod managements
ADD_HOOK_TARGET(fh_enable_mod);
ADD_HOOK_TARGET(fh_disable_mod);
ADD_HOOK_TARGET(fh_check_mod);
ADD_HOOK_TARGET(fh_get_flags);

// The rest needs has attached to a heap
// Thread management
ADD_HOOK_TARGET(fh_attach_thread);
ADD_HOOK_TARGET(fh_detach_thread);

// Misc
ADD_HOOK_TARGET(fh_get_generation_count);
ADD_HOOK_TARGET(fh_set_descriptor_loader);
ADD_HOOK_TARGET(fh_get_descriptor_loader);

// Context managements
ADD_HOOK_TARGET(fh_set_current);
ADD_HOOK_TARGET(fh_get_current);

// Context creation/deletion
ADD_HOOK_TARGET(fh_new_context);
ADD_HOOK_TARGET(fh_free_context);
ADD_HOOK_TARGET(fh_context_get_heap);

// Reference management
ADD_HOOK_TARGET(fh_dup_ref);
ADD_HOOK_TARGET(fh_del_ref);

// Object stuffs
ADD_HOOK_TARGET(fh_alloc_object);
ADD_HOOK_TARGET(fh_object_read_data);
ADD_HOOK_TARGET(fh_object_write_data);
ADD_HOOK_TARGET(fh_object_read_ref);
ADD_HOOK_TARGET(fh_object_write_ref);

// Object's type stuffs
ADD_HOOK_TARGET(fh_object_get_type_info);
ADD_HOOK_TARGET(fh_object_put_type_info);
ADD_HOOK_TARGET(fh_object_is_alias);
ADD_HOOK_TARGET(fh_object_get_type);

// Object's synchronization stuffs
ADD_HOOK_TARGET(fh_object_init_synchronization_structs);
ADD_HOOK_TARGET(fh_object_wait);
ADD_HOOK_TARGET(fh_object_wake);
ADD_HOOK_TARGET(fh_object_wake_all);
ADD_HOOK_TARGET(fh_object_lock);
ADD_HOOK_TARGET(fh_object_unlock);

// Array stuffs
ADD_HOOK_TARGET(fh_alloc_array);
// ADD_HOOK_TARGET(fh_alloc_data_array);
ADD_HOOK_TARGET(fh_array_calc_offset);
# if IS_ENABLED(CONFIG_MOD_DEBUG)
  ADD_HOOK_FUNC_AND_DECLARE(fh_array_calc_offset, HOOK_HEAD, ssize_t, debug_hook_fh_array_calc_offset_head, __FLUFFYHEAP_NONNULL(fh_array*), self, size_t, index);
# endif
ADD_HOOK_TARGET(fh_array_get_element);
ADD_HOOK_TARGET(fh_array_set_element);
ADD_HOOK_TARGET(fh_array_get_length);
# if IS_ENABLED(CONFIG_MOD_DEBUG)
  ADD_HOOK_FUNC_AND_DECLARE(fh_array_get_length, HOOK_HEAD, size_t, debug_hook_fh_array_get_length_head, __FLUFFYHEAP_NONNULL(fh_array*), self);
# endif

// Descriptor stuffs
ADD_HOOK_TARGET(fh_define_descriptor);
ADD_HOOK_TARGET(fh_get_descriptor);
ADD_HOOK_TARGET(fh_release_descriptor);
ADD_HOOK_TARGET(fh_descriptor_get_param);

// Mods
#include "api/mods/mods_hooklist.h" // IWYU pragma: keep

#ifdef STANDALONE
};
#endif

#endif

