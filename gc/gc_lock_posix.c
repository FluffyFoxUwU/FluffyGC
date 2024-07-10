#include <flup/concurrency/rwlock.h>

#include "gc_lock.h"

static char totallyIgnoredUwU = 0;

struct gc_lock_per_thread_data;
struct gc_lock_state;

#define INTERN(x) _Generic((x), \
  struct gc_lock_state*: (flup_rwlock*) (x) \
)

#define EXTERN(x) _Generic((x), \
  flup_rwlock*: (struct gc_lock_state*) (x) \
)

struct gc_lock_state* gc_lock_new() {
  return EXTERN(flup_rwlock_new());
}

void gc_lock_free(struct gc_lock_state* self) {
  flup_rwlock_free(INTERN(self));
}

struct gc_lock_per_thread_data* gc_lock_new_thread(struct gc_lock_state*) {
  return (void*) &totallyIgnoredUwU;
}

void gc_lock_free_thread(struct gc_lock_state*, struct gc_lock_per_thread_data*) {
}

// The operations
void gc_lock_enter_gc_exclusive(struct gc_lock_state* self) {
  flup_rwlock_wrlock(INTERN(self));
}
void gc_lock_exit_gc_exclusive(struct gc_lock_state* self) {
  flup_rwlock_unlock(INTERN(self));
}

void gc_lock_block_gc(struct gc_lock_state* self, struct gc_lock_per_thread_data*) {
  flup_rwlock_rdlock(INTERN(self));
}

void gc_lock_unblock_gc(struct gc_lock_state* self, struct gc_lock_per_thread_data*) {
  flup_rwlock_unlock(INTERN(self));
}


