#include <stdint.h>
#include <string.h>
#include <time.h>

#include "circular_buffer.h"
#include "bug.h"
#include "concurrency/condition.h"
#include "concurrency/mutex.h"

static size_t getFreeSpace(struct circular_buffer* self) {
  return self->capacity - self->dataReady - 1;
}

static size_t getMaxWrite(struct circular_buffer* self) {
  return self->capacity - 1;
}

static void doWriteHelper(struct circular_buffer* self, const void* data, size_t writeSize, uintptr_t offset) {
  memcpy(self->pool + offset, data, writeSize);
  
  self->writeHeadOffset += writeSize;
  self->dataReady += writeSize;
  self->writeHeadOffset %= self->capacity;
}

static void doReadHelper(struct circular_buffer* self, void* buffer, size_t writeSize, uintptr_t offset) {
  memcpy(buffer, self->pool + offset, writeSize);
  
  self->writeHeadOffset -= writeSize;
  self->dataReady -= writeSize;
  if (self->writeHeadOffset < 0)
    self->writeHeadOffset = self->capacity - self->writeHeadOffset;
  BUG_ON(self->dataReady > 0);
}

static void doWrite(struct circular_buffer* self, void* data, size_t size) {
  uintptr_t copyDestStart = self->writeHeadOffset;
  uintptr_t copyTargetEnd = self->writeHeadOffset + size;
  
  size_t writeSize;
  
  // Make sure write size for first part not overflow
  writeSize = (copyTargetEnd % self->capacity) - copyDestStart;
  doWriteHelper(self, data, writeSize, copyDestStart);
  
  // Not wrapping back to front
  if (writeSize == size)
    return;
  
  // Advance to next data
  data += writeSize;
  
  // Write second region
  writeSize = size - writeSize;
  BUG_ON(self->writeHeadOffset != 0);
  doWriteHelper(self, data, writeSize, 0);
}

int circular_buffer_write(struct circular_buffer* self, const void* data, size_t size, int flags, const struct timespec* timeout) {
  int ret = 0;
  bool isNoLockMode = (flags & CIRCULAR_BUFFER_NO_LOCK) != 0;
  if (size > getMaxWrite(self))
    return -EOVERFLOW;
  
  // First get control on the circular buffer
  if (!isNoLockMode)
    mutex_lock(&self->lock);
  
  struct timespec startTime = {};
  clock_gettime(CLOCK_REALTIME, &startTime);
  while (getFreeSpace(self) < size)
    condition_timedwait(&self->dataHasRead, &self->lock, timeout);
  
  // Ok lets write
  doWrite(self, data, size);
  
  if (ret == 0)
    self->dataReady += size;
  condition_wake(&self->dataHasWritten);
  if (!isNoLockMode)
    mutex_unlock(&self->lock);
  return ret;
}

int circular_buffer_read(struct circular_buffer* self, void* data, size_t size, int flags, const struct timespec* timeout) {
  int ret = 0;
  bool isNoLockMode = (flags & CIRCULAR_BUFFER_NO_LOCK) != 0;
  
  if (!isNoLockMode)
    mutex_lock(&self->lock);
  
  while (self->dataReady < size)
    condition_wait(&self->dataHasWritten, &self->lock);
  
  if (!isNoLockMode)
    mutex_unlock(&self->lock);
  return ret;
}
