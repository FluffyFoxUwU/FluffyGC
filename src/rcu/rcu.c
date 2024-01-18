#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rcu.h"
#include "bug.h"
#include "concurrency/condition.h"
#include "concurrency/mutex.h"
#include "panic.h"

static void blockUpdater(struct rcu_struct* self) {
  atomic_fetch_add(&self->updateBlockerCount, 1);
}

static void unblockUpdater(struct rcu_struct* self) {
  uint64_t prev = atomic_fetch_sub(&self->updateBlockerCount, 1);
  if (prev == 1)
    condition_wake(&self->wakeReclaimer);
  else if (prev < 1)
    BUG();
}

struct rcu_head* rcu_read_lock(struct rcu_struct* self) {
  blockUpdater(self);
  struct rcu_head* head = atomic_load(&self->current);
  if (head)
    atomic_fetch_add(&head->readerCount, 1);
  unblockUpdater(self);
  return head;
}

void rcu_read_unlock(struct rcu_struct* self, struct rcu_head* head) {
  uint64_t prev = 1;
  if (head)
    prev = atomic_fetch_sub(&head->readerCount, 1);
  
  if (prev == 1)
    condition_wake(&self->wakeReclaimer);
  else if (prev < 1)
    panic("Unbalanced rcu_read_*lock %lu", prev);
}

struct rcu_head* rcu_read(struct rcu_struct* self) {
  return atomic_load(&self->current);
}

struct rcu_head* rcu_exchange_and_synchronize(struct rcu_struct* self, struct rcu_head* data) {
  if (data)
    atomic_init(&data->readerCount, 0);
  
  mutex_lock(&self->updaterLock);
  struct rcu_head* watching = atomic_exchange(&self->current, data);
  
  condition_wait(&self->wakeReclaimer, &self->updaterLock, ^bool () {
    if (watching && atomic_load(&watching->readerCount) > 0)
      return true;
    
    return atomic_load(&self->updateBlockerCount) > 0;
  });
  
  mutex_unlock(&self->updaterLock);
  
  return watching;
}

int rcu_init(struct rcu_struct* self) {
  *self = (struct rcu_struct) {
    .inited = false
  };
  atomic_init(&self->updateBlockerCount, 0);
  atomic_init(&self->current, NULL);
  
  int ret = 0;
  if ((ret = mutex_init(&self->updaterLock)) < 0)
    goto failure;
  if ((ret = condition_init(&self->wakeReclaimer)) < 0)
    goto failure;
  
  self->inited = true;
  return 0;

failure:
  rcu_cleanup(self);
  return ret;
}

void rcu_cleanup(struct rcu_struct* self) {
  if (!self->inited)
    return;
  
  condition_cleanup(&self->wakeReclaimer);
  mutex_cleanup(&self->updaterLock);
}

struct rcu_struct* rcu_new() {
  struct rcu_struct* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  if (rcu_init(self) < 0)
    goto failure;
  return self;
failure:
  rcu_free(self);
  return NULL;
}

void rcu_free(struct rcu_struct* self) {
  if (!self)
    return;
  
  rcu_cleanup(self);
  free(self);
}

void rcu_memcpy(struct rcu_head* restrict dest, const struct rcu_head* restrict source, size_t rcuHeadOffset, size_t size) {
  BUG_ON(rcuHeadOffset + sizeof(struct rcu_head) >= size);
  
  const void* restrict sourceRaw = source - rcuHeadOffset;
  void* restrict destRaw = dest - rcuHeadOffset;
  
  // Copy the before rcu_head
  if (rcuHeadOffset > 0)
    memcpy(destRaw, sourceRaw, rcuHeadOffset);
  
  // Copy the after rcu_head
  size_t skipSize = rcuHeadOffset + sizeof(struct rcu_head);
  *(char*) &sourceRaw += skipSize;
  *(char*) &destRaw += skipSize;
  size -= skipSize;
  memcpy(destRaw, sourceRaw, size);
  
  *dest = (struct rcu_head) {};
  atomic_init(&dest->readerCount, 0);
}
