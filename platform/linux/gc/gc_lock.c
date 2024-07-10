#include <flup/core/logger.h>
#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#include <flup/core/panic.h>
#include <flup/concurrency/mutex.h>
#include <flup/data_structs/list_head.h>

// Futex stuffs
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "gc/gc_lock.h"

// Adapted from http://concurrencyfreaks.blogspot.com/2013/01/scalable-and-fast-read-write-locks.html
// and changed few things so there no limit on threads

enum mutator_state : uint32_t {
  // Mutator doesn't need to block GC and not being blocked
  // maps to RSTATE_UNUSED in that blog
  MUTATOR_UNUSED    = 0,
  
  // Mutator is being blocked by GC activity
  // maps to RSTATE_WAITING in that blog
  MUTATOR_WAITING   = 1,
  
  // A state in between waiting and active
  // maps to RSTATE_PREP in that blog
  MUTATOR_PREPARING = 2,
  
  // Mutator is actively blocking GC
  // maps to RSTATE_READING in that blog
  MUTATOR_ACTIVE    = 3
};

enum gc_state : uint32_t {
  // GC isn't active/does not need to block mutator
  // Maps to WSTATE_UNUSED in that blog
  GC_UNUSED         = 0,
  
  // GC is active or waiting for mutators
  // Maps to WSTATE_WRITEORWAIT in that blog
  GC_ACTIVE_OR_WAIT = 1
};

// Maps to each state in "readers_states" in that blog
struct gc_lock_per_thread_data {
  flup_list_head node;
  _Atomic(enum mutator_state) mutatorState;
};

struct gc_lock_state {
  // Maps to "readers_states" in that blog but linked list
  // instead bounded array
  flup_list_head mutatorThreadsList;
  
  // Protects the mutatorThreadsList
  flup_mutex* mutatorThreadsListLock;
  
  // Maps to "write_state" in that blog
  _Atomic(enum gc_state) gcState;
};

// Futex wrappers
static void futex_wake_one(_Atomic(uint32_t)* futexWord) {
  long ret = syscall(SYS_futex, futexWord, FUTEX_WAKE_PRIVATE, 1);
  if (ret == -1)
    flup_panic("SYS_futex with FUTEX_WAKE_PRIVATE failed: %d", errno);
}

static void futex_wake_all(_Atomic(uint32_t)* futexWord) {
  long ret = syscall(SYS_futex, futexWord, FUTEX_WAKE_PRIVATE, INT_MAX);
  if (ret == -1)
    flup_panic("SYS_futex with FUTEX_WAKE_PRIVATE failed: %d", errno);
}

// Start waiting if *futexWord equals to currentValue
static void futex_wait(_Atomic(uint32_t)* futexWord, uint32_t currentValue) {
  long ret = syscall(SYS_futex, futexWord, FUTEX_WAKE_PRIVATE, currentValue, NULL);
  if (ret == -1 && errno != EAGAIN)
    flup_panic("SYS_futex with FUTEX_WAIT_PRIVATE failed: %d", errno);
}

struct gc_lock_state* gc_lock_new() {
  struct gc_lock_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_lock_state) {
    .mutatorThreadsList = FLUP_LIST_HEAD_INIT(self->mutatorThreadsList),
    .gcState = GC_UNUSED
  };
  
  if (!(self->mutatorThreadsListLock = flup_mutex_new()))
    goto failure;
  return self;

failure:
  gc_lock_free(self);
  return NULL;
}

void gc_lock_free(struct gc_lock_state* self) {
  if (!self)
    return;
  
  flup_mutex_lock(self->mutatorThreadsListLock);
  if (!flup_list_is_empty(&self->mutatorThreadsList))
    flup_panic("Not all mutator stopped!");
  flup_mutex_unlock(self->mutatorThreadsListLock);
  
  flup_mutex_free(self->mutatorThreadsListLock);
  free(self);
}

struct gc_lock_per_thread_data* gc_lock_new_thread(struct gc_lock_state* self) {
  struct gc_lock_per_thread_data* thread = malloc(sizeof(*thread));
  if (!thread)
    return NULL;
  
  *thread = (struct gc_lock_per_thread_data) {
    .mutatorState = MUTATOR_UNUSED
  };
  
  flup_mutex_lock(self->mutatorThreadsListLock);
  flup_list_add_tail(&self->mutatorThreadsList, &thread->node);
  flup_mutex_unlock(self->mutatorThreadsListLock);
  return thread;
}

void gc_lock_free_thread(struct gc_lock_state* self, struct gc_lock_per_thread_data* thread) {
  flup_mutex_lock(self->mutatorThreadsListLock);
  flup_list_del(&thread->node);
  flup_mutex_unlock(self->mutatorThreadsListLock);
  free(thread);
}

// The operations

// Foxie couldn't find the information on the "writeLock" and "writeUnlock"
// mechanisms so Foxie try my best to guess based on "readLock" logic
void gc_lock_enter_gc_exclusive(struct gc_lock_state* self) {
  // After setting this, there will be no new MUTATOR_ACTIVE instances
  // the new one will keep waiting
  atomic_store(&self->gcState, GC_ACTIVE_OR_WAIT);
  
  // For each mutator, we'll wait until it turned either WAITING or UNUSED
  flup_mutex_lock(self->mutatorThreadsListLock);
  flup_list_head* current;
  flup_list_for_each(&self->mutatorThreadsList, current) {
    struct gc_lock_per_thread_data* thread = flup_list_entry(current, struct gc_lock_per_thread_data, node);
    
    // I get this part from the hint
    // > Notice that the numerical value of RSTATE_PREP is larger than
    // > RSTATE_WAITING and it is due to an optimization that we will
    // > see further in the code
    
    // Wait until that mutator thread is waiting/unused
    while (atomic_load(&thread->mutatorState) > MUTATOR_WAITING)
      futex_wait(&thread->mutatorState, MUTATOR_PREPARING);
  }
  flup_mutex_unlock(self->mutatorThreadsListLock);
}

void gc_lock_exit_gc_exclusive(struct gc_lock_state* self) {
  atomic_store(&self->gcState, GC_UNUSED);
  futex_wake_all(&self->gcState);
}

// Maps to "readLock" in that blog
void gc_lock_block_gc(struct gc_lock_state* self, struct gc_lock_per_thread_data* thread) {
  atomic_store(&thread->mutatorState, MUTATOR_PREPARING);
  while (atomic_load(&self->gcState) == GC_ACTIVE_OR_WAIT) {
    atomic_store(&thread->mutatorState, MUTATOR_WAITING);
    
    // GC might want to know this interesting MUTATOR_WAITING state
    futex_wake_one(&thread->mutatorState);
    
    while (atomic_load(&self->gcState) == GC_ACTIVE_OR_WAIT)
      futex_wait(&self->gcState, GC_ACTIVE_OR_WAIT);
    
    atomic_store(&thread->mutatorState, MUTATOR_PREPARING);
  }
  atomic_store(&thread->mutatorState, MUTATOR_ACTIVE);
}

// Maps to "readUnlock" in that blog
void gc_lock_unblock_gc(struct gc_lock_state* self, struct gc_lock_per_thread_data* thread) {
  atomic_store(&thread->mutatorState, MUTATOR_UNUSED);
  
  // GC might want to know this interesting MUTATOR_UNUSED state
  if (atomic_load(&self->gcState) == GC_ACTIVE_OR_WAIT)
    futex_wake_one(&thread->mutatorState);
}


