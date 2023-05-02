#include <pthread.h>

#include "concurrency/rwlock.h"
#include "managed_heap.h"
#include "context.h"

void managed_heap_gc_safepoint() {
}

void managed_heap__block_gc(struct managed_heap* self) {
  rwlock_rdlock(&self->gcLock);
}

void managed_heap__unblock_gc(struct managed_heap* self) {
  rwlock_unlock(&self->gcLock);
}
