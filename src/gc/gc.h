#ifndef _headers_1682059498_FluffyGC_gc
#define _headers_1682059498_FluffyGC_gc

#include <stdint.h>
#include <threads.h>

#include "attributes.h"
#include "gc/gc_statistic.h"
#include "gc_flags.h"
#include "concurrency/rwulock.h"

#define GC_MAX_GENERATIONS 3

extern thread_local struct gc_struct* gc_current;

struct object;
struct root_ref;
struct context;
struct generation;

struct gc_hooks {
  // These are callback for various spot that some GC implementation may
  // be interested in
  
  // Called with object which was overwritten
  void (*postWriteBarrier)(struct object*); 
  
  // Called with object which was read and after forwarded 
  // addresses adjusted
  void (*postReadBarrier)(struct object*);
  
  int (*postObjectAlloc)(struct object*);
  void (*preObjectDealloc)(struct object*);
  
  int (*postContextInit)(struct context*);
  void (*preContextCleanup)(struct context*);
  
  void (*onSafepoint)();
  
  // These return reclaimed bytes
  size_t (*collect)(struct generation*);
  
  void (*free)(struct gc_hooks*);
};

ATTRIBUTE_USED()
static int ___gc_callback_nop_object(struct object*) {
  return 0;
}

ATTRIBUTE_USED()
static void ___gc_callback_nop_object2(struct object*) {
  return;
}

ATTRIBUTE_USED()
static void ___gc_callback_nop_context(struct context*) {
  return;
}

ATTRIBUTE_USED()
static int ___gc_callback_nop_context2(struct context*) {
  return 0;
}

ATTRIBUTE_USED()
static void ___gc_callback_nop_void(struct gc_hooks*) {
  return;
}

ATTRIBUTE_USED()
static void ___gc_callback_nop_void2() {
  return;
}

#define GC_HOOKS_DEFAULT \
  .postWriteBarrier = ___gc_callback_nop_object2, \
  .postReadBarrier = ___gc_callback_nop_object2, \
   \
  .postObjectAlloc = ___gc_callback_nop_object, \
  .preObjectDealloc = ___gc_callback_nop_object2, \
   \
  .postContextInit = ___gc_callback_nop_context2, \
  .preContextCleanup = ___gc_callback_nop_context, \
   \
  .onSafepoint = ___gc_callback_nop_void2

enum gc_algorithm {
  GC_UNKNOWN,
  GC_NOP_GC,
  GC_SERIAL_GC
};

struct gc_struct {
  struct gc_hooks* hooks;
  enum gc_algorithm algoritmn;
  struct rwulock gcLock;
  
  struct gc_statistic stat;
};

int gc_generation_count(enum gc_algorithm algo, gc_flags gcFlags);
bool gc_use_fast_on_gen(enum gc_algorithm algo, gc_flags gcFlags, int genID);

struct gc_struct* gc_new(enum gc_algorithm algo, gc_flags gcFlags);
void gc_free(struct gc_struct* self);

// Always blocking, caller responsible to handle multi calls
// caller should be in
// NULL invokes FullGC
void gc_start(struct gc_struct* self, struct generation* generation);

#define gc_safepoint() gc_current->hooks->onSafepoint()

// Cannot be nested
// consider using context_(un)block_gc functions instead

#define gc_block(self) do { \
  rwulock_rdlock(&(self)->gcLock); \
} while (0)
#define gc_unblock(self) do { \
  rwulock_unlock(&(self)->gcLock); \
} while (0)

bool gc_upgrade_to_gc_mode(struct gc_struct* self);
void gc_downgrade_from_gc_mode(struct gc_struct* self);

void gc_for_each_root_entry(struct gc_struct* self, void (^iterator)(struct root_ref*));

#endif

