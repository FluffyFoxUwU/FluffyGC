#ifndef _headers_1689844559_FluffyGC_hook_list
#define _headers_1689844559_FluffyGC_hook_list

#include "FluffyHeap.h"
#include "attributes.h"
#include "hook.h"

#include "api/mods/debug/debug.h"

// Just to let include cleaner shut up
#ifdef __FLUFFYHEAP_EXPORT
#endif

#ifndef ADD_HOOK_TARGET
# define STANDALONE
# define ADD_HOOK_TARGET(target)
# define ADD_HOOK_FUNC(target, location, func) hook_register(target, location, func)
#endif

#ifdef STANDALONE
ATTRIBUTE_USED()
static void uwu() {
#endif

// Keep the include alive
(void) api_mod_debug_init;

//////////////////////////////
// Add new hook target here //
//////////////////////////////

// Example
// ADD_HOOK_TARGET(foo);
// ADD_HOOK_FUNC(foo, );

// Heap management
ADD_HOOK_TARGET(fh_new);
ADD_HOOK_TARGET(fh_free);

// Thread management
ADD_HOOK_TARGET(fh_attach_thread);
ADD_HOOK_TARGET(fh_detach_thread);

// Misc
ADD_HOOK_TARGET(fh_get_generation_count);
ADD_HOOK_TARGET(fh_set_descriptor_loader);
ADD_HOOK_TARGET(fh_get_descriptor_loader);

// Mod managements
ADD_HOOK_TARGET(fh_enable_mod);
ADD_HOOK_TARGET(fh_disable_mod);
ADD_HOOK_TARGET(fh_check_mod);
ADD_HOOK_TARGET(fh_get_flags);

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
ADD_HOOK_TARGET(fh_array_get_element);
ADD_HOOK_TARGET(fh_array_set_element);
ADD_HOOK_TARGET(fh_array_get_length);
  ADD_HOOK_FUNC(fh_array_get_length, HOOK_HEAD, api_mod_debug_hook_fh_array_get_length);

// Descriptor stuffs
ADD_HOOK_TARGET(fh_define_descriptor);
ADD_HOOK_TARGET(fh_get_descriptor);
ADD_HOOK_TARGET(fh_release_descriptor);
ADD_HOOK_TARGET(fh_descriptor_get_param);

#ifdef STANDALONE
}
#endif

#define UWU_SO_THAT_UNUSED_COMPLAIN_SHUT_UP

#endif

