#ifndef _headers_1686390737_FluffyGC_FluffyHeap
#define _headers_1686390737_FluffyGC_FluffyHeap

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define __FLUFFYHEAP_EXPORT extern

#if __clang__
# define __FLUFFYHEAP_NONNULL(t) t _Nonnull
# define __FLUFFYHEAP_NULLABLE(t) t _Nullable
#elif __GNUC__
# define __FLUFFYHEAP_NONNULL(t) t __attribute__((nonnull))
# define __FLUFFYHEAP_NULLABLE(t) t
#endif

// Types
typedef struct fh_200d21fd_f92b_41e0_9ea3_32eaf2b4e455 fluffyheap;
typedef struct fh_2530dc70_b8d3_41d5_808a_922504f90ff6 fh_context;
typedef struct fh_e96bee3a_e673_4a27_97e3_a365dc3d9d53 fh_object;
typedef struct fh_7f49677c_68b4_40ea_96c7_db3f6176c5dc fh_array;
typedef struct fh_fc200f7a_174f_4882_9fa4_1205af13686e fh_descriptor;

typedef __FLUFFYHEAP_NULLABLE(fh_descriptor*) (*fh_descriptor_loader)(const char* name, void* udata);

enum fh_gc_hint {
  FH_GC_BALANCED,
  FH_GC_LOW_LATENCY,
  FH_GC_HIGH_THROUGHPUT
};

typedef struct {
  enum fh_gc_hint hint;
  size_t generationCount;
  size_t* generationSizes;
} fh_param;

enum fh_object_type {
  FH_TYPE_NORMAL,
  FH_TYPE_ARRAY,
};

enum fh_reference_strength {
  FH_REF_STRONG
};

typedef struct {
  const char* name;
  size_t offset;
  const char* dataType;
  enum fh_reference_strength strength;
} fh_descriptor_field;

typedef struct {
  enum fh_object_type type;
  size_t size;
  size_t alignment;

  // For arrays this only 1 long
  fh_descriptor_field* fields;
} fh_descriptor_param;

#define FH_FIELD(type, member, _dataType, refStrength) \
  { \
    .name = #member, \
    .offset = offsetof(type, member), \
    .dataType = (_dataType), \
    .strength = (refStrength) \
  }
#define FH_FIELD_END() {.name = NULL}

// Heap stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fluffyheap*) fh_new(__FLUFFYHEAP_NONNULL(fh_param*) param);
__FLUFFYHEAP_EXPORT void fh_free(__FLUFFYHEAP_NONNULL(fluffyheap*) self);

__FLUFFYHEAP_EXPORT int fh_attach_thread(__FLUFFYHEAP_NONNULL(fluffyheap*) self);
__FLUFFYHEAP_EXPORT int fh_detach_thread(__FLUFFYHEAP_NONNULL(fluffyheap*) self);

__FLUFFYHEAP_EXPORT int fh_get_generation_count(enum fh_gc_hint hint);

__FLUFFYHEAP_EXPORT void fh_set_descriptor_loader(__FLUFFYHEAP_NONNULL(fluffyheap*) self, __FLUFFYHEAP_NULLABLE(fh_descriptor_loader) loader);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor_loader) fh_get_descriptor_loader(__FLUFFYHEAP_NONNULL(fluffyheap*) self);

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

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_context*) fh_new_context(__FLUFFYHEAP_NONNULL(fluffyheap*) heap);
__FLUFFYHEAP_EXPORT void fh_free_context(__FLUFFYHEAP_NONNULL(fluffyheap*) heap, __FLUFFYHEAP_NONNULL(fh_context*) self);

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NONNULL(fluffyheap*) fh_context_get_heap(__FLUFFYHEAP_NONNULL(fh_context*) self);

// Root stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_dup_ref(__FLUFFYHEAP_NULLABLE(fh_object*) ref);
__FLUFFYHEAP_EXPORT void fh_del_ref(__FLUFFYHEAP_NONNULL(fh_object*) ref);

// Objects stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_alloc_object(__FLUFFYHEAP_NONNULL(fh_descriptor*) desc);

__FLUFFYHEAP_EXPORT void fh_object_read_data(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(void*) buffer, size_t offset, size_t size);
__FLUFFYHEAP_EXPORT void fh_object_write_data(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NONNULL(const void*) buffer, size_t offset, size_t size);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_object_read_ref(__FLUFFYHEAP_NONNULL(fh_object*) self, size_t offset);
__FLUFFYHEAP_EXPORT void fh_object_write_ref(__FLUFFYHEAP_NONNULL(fh_object*) self, size_t offset, __FLUFFYHEAP_NULLABLE(fh_object*) data);
__FLUFFYHEAP_EXPORT void fh_object_wait(__FLUFFYHEAP_NONNULL(fh_object*) self, __FLUFFYHEAP_NULLABLE(const struct timespec*) timeout);
__FLUFFYHEAP_EXPORT void fh_object_wake(__FLUFFYHEAP_NONNULL(fh_object*) self);
__FLUFFYHEAP_EXPORT void fh_object_wake_all(__FLUFFYHEAP_NONNULL(fh_object*) self);
__FLUFFYHEAP_EXPORT void fh_object_lock(__FLUFFYHEAP_NONNULL(fh_object*) self);
__FLUFFYHEAP_EXPORT void fh_object_unlock(__FLUFFYHEAP_NONNULL(fh_object*) self);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NONNULL(fh_descriptor*) fh_object_get_descriptor(__FLUFFYHEAP_NONNULL(fh_object*) self);

// Arrays stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_array*) fh_alloc_array(__FLUFFYHEAP_NONNULL(fh_descriptor*) elementType, size_t length);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_array*) fh_alloc_data_array(size_t alignment, size_t length, size_t size);

__FLUFFYHEAP_EXPORT ssize_t fh_array_calc_offset(__FLUFFYHEAP_NONNULL(fh_array*) self, size_t index);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_object*) fh_array_get_element(__FLUFFYHEAP_NONNULL(fh_array*) self, size_t index);
__FLUFFYHEAP_EXPORT void fh_array_set_element(__FLUFFYHEAP_NONNULL(fh_array*) self, size_t index, __FLUFFYHEAP_NULLABLE(fh_object*) object);

// Descriptors stuff
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_define_descriptor(const char* name, fh_descriptor_param* parameter);
__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_get_descriptor(const char* name, bool dontInvokeLoader);
__FLUFFYHEAP_EXPORT void fh_release_descriptor(__FLUFFYHEAP_NULLABLE(fh_descriptor*) desc);

#endif

