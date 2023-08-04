#include <string.h>
#include <errno.h>

#include "circular_buffer.h"
#include "bug.h"
#include "concurrency/condition.h"
#include "concurrency/mutex.h"

// Most codes are reformatted from (comments preserved)
// https://github.com/noahcroit/RingBuffer_and_DataFraming_C/blob/master/circularBuffer.c

enum buf_state {
  BUF_STATE_EMPTY,
  BUF_STATE_FULL,
  BUF_STATE_R_MORE_THAN_F,
  BUF_STATE_R_LESS_THAN_F
};

static bool startCritical(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  if (flags & CIRCULAR_BUFFER_NO_LOCK)
    return true;
  
  if (flags & CIRCULAR_BUFFER_NONBLOCK) {
    // Is going to sleep
    if (!mutex_lock2(&self->lock, MUTEX_LOCK_TIMED, abstimeout))
      return false;
  } else {
    mutex_lock(&self->lock);
  }
  
  return true;
}

static void endCritical(struct circular_buffer* self, int flags) {
  if (!(flags & CIRCULAR_BUFFER_NO_LOCK))
    mutex_unlock(&self->lock);
}

int circular_buffer_is_empty(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  if (!startCritical(self, flags, abstimeout))
    return -EAGAIN;
  
  bool res = (self->r == -1 && self->f == -1);
  endCritical(self, flags);
  return res;
}

int circular_buffer_is_full(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  if (!startCritical(self, flags, abstimeout))
    return -EAGAIN;
  
  bool res = (self->f == self->r) && (self->f != -1);
  endCritical(self, flags);
  return res;
}

static enum buf_state getBufState(struct circular_buffer* self, const struct timespec* abstimeout) {
  enum buf_state ret;
  if (circular_buffer_is_empty(self, CIRCULAR_BUFFER_NO_LOCK, abstimeout))
    ret = BUF_STATE_EMPTY;         //Empty state
  else if (circular_buffer_is_empty(self, CIRCULAR_BUFFER_NO_LOCK, abstimeout))
    ret = BUF_STATE_FULL;          //Full state
  else if(self->r  > self->f)
    ret = BUF_STATE_R_MORE_THAN_F; //rear > front
  else if(self->r  < self->f)
    ret = BUF_STATE_R_LESS_THAN_F; //rear < front
  else
    BUG();
  return ret;
}

int circular_buffer_write(struct circular_buffer* self, int flags, const void* data, size_t size, const struct timespec* abstimeout) {
  if (!startCritical(self, flags, abstimeout))
    return -EAGAIN;
  
  __block enum buf_state bufferState;
  int ret = condition_wait2(&self->dataHasRead, &self->lock, CONDITION_WAIT_TIMED, abstimeout, ^bool () {
    return (bufferState = getBufState(self, abstimeout)) == BUF_STATE_FULL;  
  });
  
  if (ret < 0)
    goto failed_waiting_to_have_space;
  
  switch (bufferState) {
    case BUF_STATE_EMPTY: //Empty state
      /* Change buffer to non-empty state (buffer is reset) */
      self->r = 0;
      self->f = 0;
    case BUF_STATE_R_MORE_THAN_F:
      if ((self->r + size) <= self->bufferSize) {
        /**  memcpy() between buffer and enqueue data
          *  start from rear:r to r + enqueueSize
          *  when r + enqueueSize does not exceed end-of-buffer   (Not wrapping)
          *  then, copy only 1 section
          */
        
        memcpy(self->buf + self->r, data, size);
        self->r += size;
        self->r %= self->bufferSize;
      } else {
        /**  memcpy() between buffer and enqueue data
          *  start from rear:r to r + enqueueSize
          *  when r + enqueueSize exceed end-of-buffer            (Wrapping)
          *  then, copy with 2 sections
          */

        /* 1st section copy (r to end-of-buffer part) */
        size_t firstWriteSize = self->bufferSize - self->r;
        memcpy(self->buf + self->r, data, firstWriteSize);
        data += firstWriteSize;
        
        /* 2nd section copy (wrapping part) */
        if (size + self->r - self->bufferSize <= self->f) {
          // No overwritten occur
          memcpy(self->buf, data, size + self->r - self->bufferSize);
          self->r += size;
          self->r %= self->bufferSize;
        } else {
          // TODO: Do something (Overwriting detected!)
          BUG();
        }
      }
      break;
    case BUF_STATE_R_LESS_THAN_F:     //rear < front
      if (self->r + size <= self->f) {
        /**  memcpy() between buffer and enqueue data
          *  start from rear:r to r + enqueueSize
          *  when r + enqueueSize is not exceed front : f   (Overwritten do not occur)
          */
        memcpy(self->buf + self->r, data, size);
        self->r += size;
        self->r %= self->bufferSize;
      } else {
        // TODO: Do something (Overwriting detected!)
        BUG();
      }
      break;
    case BUF_STATE_FULL:
      BUG();
  }
  
failed_waiting_to_have_space:
  endCritical(self, flags);
  return ret;
}
