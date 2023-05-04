#ifndef _headers_1682059498_FluffyGC_gc
#define _headers_1682059498_FluffyGC_gc

#include <threads.h>

#include "attributes.h"

extern thread_local struct gc_struct* gc_current;

struct object;
struct context;

typedef int (*gc_object_callback)(struct object*);
typedef int (*gc_context_callback)(struct context*);

struct gc_hooks {
  // These are callback for various spot that some GC implementation may
  // be interested in
  
  // Called with object which was overwritten
  gc_object_callback postWriteBarrier; 
  
  // Called with object which was read and after forwarded 
  // addresses adjusted
  gc_object_callback postReadBarrier;
  
  gc_object_callback postObjectAlloc;
  gc_object_callback preObjectDealloc;
  
  gc_context_callback postContextInit;
  gc_context_callback preContextCleanup;
  
  gc_context_callback onSafepoint;
  
  void (*free)(struct gc_hooks*);
};

ATTRIBUTE_USED()
static int ___gc_callback_nop_object(struct object*) {
  return 0;
}

ATTRIBUTE_USED()
static int ___gc_callback_nop_context(struct context*) {
  return 0;
}

ATTRIBUTE_USED()
static void ___gc_callback_nop_void(struct gc_hooks*) {
  return;
}

#define GC_HOOKS_DEFAULT \
  .postWriteBarrier = ___gc_callback_nop_object, \
  .postReadBarrier = ___gc_callback_nop_object, \
  .postObjectAlloc = ___gc_callback_nop_object, \
  .preObjectDealloc = ___gc_callback_nop_object, \
  .postContextInit = ___gc_callback_nop_context, \
  .preContextCleanup = ___gc_callback_nop_context, \
  .onSafepoint = ___gc_callback_nop_context, \
  .free = ___gc_callback_nop_void

enum gc_algorithm {
  GC_UNKNOWN,
  GC_NOP_GC
};

struct gc_struct {
  struct gc_hooks* hooks;
  enum gc_algorithm algoritmn;
};

struct gc_struct* gc_new(enum gc_algorithm algo, int gcFlags);
void gc_free(struct gc_struct* self);

// 0 stands for young GC
// 1 stands for old GC
void gc_start(int generation);

#endif

