#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "circular_buffer.h"
#include "bug.h"
#include "concurrency/condition.h"
#include "concurrency/mutex.h"
#include "util.h"

// Most codes are reformatted from (comments preserved)
// https://github.com/noahcroit/RingBuffer_and_DataFraming_C/blob/8d41e99a78e1977ae71968a3d4cb3cb607bc770a/circularBuffer.c

enum buf_state {
  BUF_STATE_EMPTY,
  BUF_STATE_R_MORE_THAN_F,
  BUF_STATE_R_LESS_THAN_F
};

static int startCritical(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  int ret = 0;
  if (flags & CIRCULAR_BUFFER_NO_LOCK)
    return 0;
  
  if (flags & CIRCULAR_BUFFER_NONBLOCK) {
    ret = mutex_lock2(&self->lock, MUTEX_LOCK_NONBLOCK, NULL);
  } else if (flags & CIRCULAR_BUFFER_WITH_TIMEOUT) {
    ret = mutex_lock2(&self->lock, MUTEX_LOCK_TIMED, abstimeout);
  } else {
    mutex_lock(&self->lock);
  }
  
  return ret;
}

static void endCritical(struct circular_buffer* self, int flags) {
  if (!(flags & CIRCULAR_BUFFER_NO_LOCK))
    mutex_unlock(&self->lock);
}

int circular_buffer_is_empty(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  int ret = 0;
  if ((ret = startCritical(self, flags, abstimeout)) < 0)
    return ret;
  
  ret = self->dataInBuffer == 0;
  endCritical(self, flags);
  return ret;
}

int circular_buffer_is_full(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  int ret = 0;
  if ((ret = startCritical(self, flags, abstimeout)) < 0)
    return ret;
  
  ret = self->dataInBuffer == self->bufferSize;
  endCritical(self, flags);
  return ret;
}

static enum buf_state getBufState(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  enum buf_state ret;
  if (circular_buffer_is_empty(self, flags, abstimeout))
    ret = BUF_STATE_EMPTY; //Buffer is empty
  else if (self->r > self->f)
    ret = BUF_STATE_R_MORE_THAN_F; //rear > front
  else if (self->r <= self->f)
    ret = BUF_STATE_R_LESS_THAN_F; //rear < front
  else
    BUG();
  return ret;
}

static int getWaitFlags(int flags) {
  int waitFlags = 0;
  if (flags & CIRCULAR_BUFFER_WITH_TIMEOUT)
    waitFlags |= CONDITION_WAIT_TIMED;
  return waitFlags;
} 

static size_t internalWrite(struct circular_buffer* self, int flags, const void** data, size_t* size, const struct timespec* abstimeout) {
  if (*size == 0) {
    errno = 0;
    return 0;
  }
  int ret = 0;
  size_t writeSize = 0;
  
  enum buf_state bufferState;
  ret = condition_wait2(&self->dataHasRead, &self->lock, getWaitFlags(flags), abstimeout, ^bool () {
    return circular_buffer_is_full(self, flags | CIRCULAR_BUFFER_NO_LOCK, abstimeout);
  });
  if (ret < 0)
    goto failed_waiting_to_have_space;
  
  // Compute amount to write
  writeSize = MIN(self->bufferSize - self->dataInBuffer, *size);
  bufferState = getBufState(self, flags, abstimeout);
  
  switch (bufferState) {
    // Fallthrough to next
    case BUF_STATE_EMPTY:
    case BUF_STATE_R_MORE_THAN_F:
      if ((self->r + writeSize) <= self->bufferSize) {
        /**  memcpy() between buffer and enqueue data
          *  start from rear:r to r + enqueueSize
          *  when r + enqueueSize does not exceed end-of-buffer   (Not wrapping)
          *  then, copy only 1 section
          */
        
        memcpy(self->buf + self->r, *data, writeSize);
        self->r += writeSize;
        self->r %= self->bufferSize;
      } else {
        /**  memcpy() between buffer and enqueue data
          *  start from rear:r to r + enqueueSize
          *  when r + enqueueSize exceed end-of-buffer            (Wrapping)
          *  then, copy with 2 sections
          */

        /* 1st section copy (r to end-of-buffer part) */
        size_t firstWriteSize = self->bufferSize - self->r;
        memcpy(self->buf + self->r, *data, firstWriteSize);
        *data += firstWriteSize;
        
        /* 2nd section copy (wrapping part) */
        if (writeSize + self->r - self->bufferSize <= self->f) {
          // No overwritten occur
          memcpy(self->buf, *data, writeSize + self->r - self->bufferSize);
          self->r += writeSize;
          self->r %= self->bufferSize;
        } else {
          // Overwriting: Already set max bound earlier
          BUG();
        }
      }
      break;
    case BUF_STATE_R_LESS_THAN_F:     //rear < front
      if (self->r + writeSize <= self->f) {
        /**  memcpy() between buffer and enqueue data
          *  start from rear:r to r + enqueueSize
          *  when r + enqueueSize is not exceed front : f   (Overwritten do not occur)
          */
        memcpy(self->buf + self->r, *data, writeSize);
        self->r += writeSize;
        self->r %= self->bufferSize;
      } else {
        // Overwriting: Already set max bound earlier
        BUG();
      }
      break;
  }
  
  *data += writeSize;
  *size -= writeSize;
  self->dataInBuffer += writeSize;
  
  condition_wake_all(&self->dataHasWritten);
failed_waiting_to_have_space:
  if (ret < 0) {
    errno = -ret;
    writeSize = 0;
  }
  return writeSize;
}

// Returns bytes read
// Sets errno on error or sets errno to 0 on sucess
// `data` and `size` updated accordingly
// to amount actually read
static size_t internalRead(struct circular_buffer* self, int flags, void** data, size_t* size, const struct timespec* abstimeout) {
  if (*size == 0) {
    errno = 0;
    return 0;
  }
  
  int ret = 0;
  size_t readSize = 0;
  enum buf_state bufferState;
  ret = condition_wait2(&self->dataHasWritten, &self->lock, getWaitFlags(flags), abstimeout, ^bool () {
    return circular_buffer_is_empty(self, flags | CIRCULAR_BUFFER_NO_LOCK, abstimeout);
  });
  if (ret < 0)
    goto failed_waiting_to_have_data;
  
  // Compute actual readable size
  bufferState = getBufState(self, flags, abstimeout);  
  readSize = MIN(self->dataInBuffer, *size);
  
  switch (bufferState) {
    case BUF_STATE_R_MORE_THAN_F: //rear > front
      if (readSize <= (self->r - self->f)) {
        /**  memcpy() between buffer and dequeue data
          *  start from front:f to f + dequeueSize
          *  when f + dequeueSize does not exceed r
          */
        memcpy(*data, self->buf + self->f, readSize);
        self->f += readSize;
        self->f %= self->bufferSize;
      } else {
        // Overreading detected but already set max size earlier
        BUG();
      }
      break;
    case BUF_STATE_R_LESS_THAN_F:
      if (self->f + readSize <= self->bufferSize) {
        /**  memcpy() between buffer and dequeue data
          *  start from front:f to f + dequeueSize
          *  when f + dequeueSize is not exceed end-of-buffer   (Not Wrapping)
          *  then, dequeue only 1 section
          */
        
        memcpy(*data, self->buf + self->f, readSize);
        self->f += readSize;
        self->f %= self->bufferSize;
      } else {
        /**  memcpy() between buffer and dequeue data
          *  start from front:f to f + dequeueSize
          *  when f + dequeueSize is exceed end-of-buffer   (Wrapping)
          *  then, dequeue with 2 sections
          */
        /* 1st section copy */
        memcpy(data, self->buf + self->f, self->bufferSize - self->f);
        data += self->bufferSize - self->f;
        
        /* 2nd section copy (wrapping part) */
        // No overwritten occur
        if(readSize + self->f - self->bufferSize <= self->r) {
          memcpy(*data, self->buf, readSize + self->f - self->bufferSize);
          self->f += readSize;
          self->f %= self->bufferSize;
        } else {
          // Overreading detected but already set max size earlier
          BUG();
        }
      }
      break;
    case BUF_STATE_EMPTY:
      BUG();
  }
  
  *data += readSize;
  *size -= readSize;
  self->dataInBuffer -= readSize;
  
  condition_wake_all(&self->dataHasRead);
failed_waiting_to_have_data:
  if (ret < 0) {
    errno = -ret;
    readSize = 0;
  }
  return readSize;
}

size_t circular_buffer_read(struct circular_buffer* self, int flags, void* data, size_t size, const struct timespec* abstimeout) {
  int ret = 0;
  if ((ret = startCritical(self, flags, abstimeout)) < 0)
    return ret;
  
  size_t leftToProcess = size;
  size_t processedSize = 0;
  void* currentData = data;
  
  while (leftToProcess > 0) {
    errno = 0;
    processedSize += internalRead(self, flags | CIRCULAR_BUFFER_NO_LOCK, &currentData, &leftToProcess, abstimeout);
    if (errno > 0)
      break;
  }
  
  endCritical(self, flags);
  return processedSize;
}

size_t circular_buffer_write(struct circular_buffer* self, int flags, const void* data, size_t size, const struct timespec* abstimeout) {
  int ret = 0;
  if ((ret = startCritical(self, flags, abstimeout)) < 0)
    return ret;
  
  size_t leftToProcess = size;
  size_t processedSize = 0;
  const void* currentData = data;
  
  while (leftToProcess > 0) {
    errno = 0;
    processedSize += internalWrite(self, flags | CIRCULAR_BUFFER_NO_LOCK, &currentData, &leftToProcess, abstimeout);
    if (errno > 0)
      break;
  }
  
  endCritical(self, flags);
  return processedSize;
}

struct circular_buffer* circular_buffer_new(size_t size) {
  void* buffer = malloc(size);
  if (!buffer)
    return NULL;
  
  struct circular_buffer* self = circular_buffer_new_from_ptr(size, buffer);
  if (!self)
    goto failure;
  self->allocPtr = buffer;
  return self;

failure:
  circular_buffer_free(self);
  free(buffer);
  return NULL;
}

struct circular_buffer* circular_buffer_new_from_ptr(size_t size, void* ptr) {
  struct circular_buffer* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct circular_buffer) {
    .bufferSize = size,
    .buf = ptr,
    .r = 0,
    .f = 0
  };
  return self;
}

void circular_buffer_free(struct circular_buffer* self) {
  if (!self)
    return;
  
  free(self->allocPtr);
  free(self);
}
