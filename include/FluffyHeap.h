#ifndef _headers_1686390737_FluffyGC_FluffyHeap
#define _headers_1686390737_FluffyGC_FluffyHeap

#include <stdint.h>
#define __FLUFFYHEAP_EXPORT extern

#if __clang__
# define __FLUFFYHEAP_NONNULL(t) t _Nonnull
# define __FLUFFYHEAP_NULLABLE(t) t _Nullable
#elif __GNUC__
# define __FLUFFYHEAP_NONNULL(t) t __attribute__((nonnull))
# define __FLUFFYHEAP_NULLABLE(t) t
#endif

// Types
typedef struct fh_200d21fd_f92b_41e0_9ea3_32eaf2b4e455 fh_heap;
typedef struct fh_2530dc70_b8d3_41d5_808a_922504f90ff6 fh_context;
typedef struct fh_e96bee3a_e673_4a27_97e3_a365dc3d9d53 fh_object;

typedef struct {
  uint64_t gcFlags;
} fh_param;

// Heap stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_heap*) fh_new(__FLUFFYHEAP_NONNULL(fh_param*) param);
__FLUFFYHEAP_EXPORT void fh_free(__FLUFFYHEAP_NONNULL(fh_heap*) self);

__FLUFFYHEAP_EXPORT int fh_attach_thread(__FLUFFYHEAP_NONNULL(fh_heap*) self);
__FLUFFYHEAP_EXPORT int fh_detach_thread(__FLUFFYHEAP_NONNULL(fh_heap*) self);

// Mods stuff
enum fh_mod {
  FH_MOD_UNKNOWN  = 0x0000,
  FH_MOD_DMA      = 0x0001,
  FH_MOD_ROBUST   = 0x0002,
};

__FLUFFYHEAP_EXPORT int fh_enable_mod(enum fh_mod mod, int flags);
__FLUFFYHEAP_EXPORT void fh_disable_mod(enum fh_mod mod);
__FLUFFYHEAP_EXPORT bool fh_check_mod(enum fh_mod mod, int flags);

// Context stuff
__FLUFFYHEAP_EXPORT int fh_set_current(__FLUFFYHEAP_NULLABLE(fh_context*) context);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_context*) fh_get_current();

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_context*) fh_new_context(__FLUFFYHEAP_NONNULL(fh_heap*) heap);
__FLUFFYHEAP_EXPORT void fh_free_context(__FLUFFYHEAP_NONNULL(fh_heap*) heap, __FLUFFYHEAP_NONNULL(fh_context*) self);

// Root stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_dup_ref(__FLUFFYHEAP_NULLABLE(fh_object*) ref);
__FLUFFYHEAP_EXPORT void fh_del_ref(__FLUFFYHEAP_NONNULL(fh_object*) ref);

#endif

