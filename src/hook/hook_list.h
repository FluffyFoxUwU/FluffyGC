#ifndef _headers_1689844559_FluffyGC_hook_list
#define _headers_1689844559_FluffyGC_hook_list

#ifndef ADD_HOOK_TARGET
# define ADD_HOOK_TARGET(target)
#endif

//////////////////////////////
// Add new hook target here //
//////////////////////////////

int foo(int arg1, int arg2);
ADD_HOOK_TARGET(foo);

#define UWU_SO_THAT_UNUSED_COMPLAIN_SHUT_UP

#endif

