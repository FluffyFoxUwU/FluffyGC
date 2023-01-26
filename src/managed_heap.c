#include <pthread.h>

#include "managed_heap.h"
#include "context.h"

void managed_heap_gc_safepoint() {
}

void managed_heap_block_gc(struct managed_heap* self) {
  pthread_rwlock_rdlock(&self->gcLock);
}

void managed_heap_unblock_gc(struct managed_heap* self) {
  pthread_rwlock_unlock(&self->gcLock);
}
