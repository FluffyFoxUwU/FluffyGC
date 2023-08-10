#include <string.h>
#include <errno.h>

#include "circular_buffer.h"
#include "bug.h"
#include "concurrency/condition.h"
#include "concurrency/mutex.h"

// Most codes are reformatted from (comments preserved)
// https://github.com/noahcroit/RingBuffer_and_DataFraming_C/blob/8d41e99a78e1977ae71968a3d4cb3cb607bc770a/circularBuffer.c

enum buf_state {
  BUF_STATE_EMPTY,
  BUF_STATE_FULL,
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

static enum buf_state getBufState(struct circular_buffer* self, int flags, const struct timespec* abstimeout) {
  enum buf_state ret;
  if (circular_buffer_is_empty(self, flags | CIRCULAR_BUFFER_NO_LOCK, abstimeout))
    ret = BUF_STATE_EMPTY;         //Empty state
  else if(self->r  > self->f)
    ret = BUF_STATE_R_MORE_THAN_F; //rear > front
  else if(self->r  < self->f)
    ret = BUF_STATE_R_LESS_THAN_F; //rear < front
  else
    BUG();
  return ret;
}

int circular_buffer_write(struct circular_buffer* self, int flags, const void* data, size_t size, const struct timespec* abstimeout) {
  int ret = 0;
  if ((ret = startCritical(self, flags, abstimeout)) < 0)
    return ret;
  
  __block enum buf_state bufferState;
  ret = condition_wait2(&self->dataHasRead, &self->lock, CONDITION_WAIT_TIMED, abstimeout, ^bool () {
    if (circular_buffer_is_full(self, flags & CIRCULAR_BUFFER_NO_LOCK, abstimeout))
      return true;
    
    bufferState = getBufState(self, flags, abstimeout);  
    return false;
  });
  
  if (ret < 0)
    goto failed_waiting_to_have_space;
  
  if (bufferState == BUF_STATE_EMPTY) {
    //Empty state
    /* Change buffer to non-empty state (buffer is reset) */
    self->r = 0;
    self->f = 0;
  }
  
  switch (bufferState) {
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
          // TODO: Make it wait until reader consumes
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
        // TODO: Make it wait until reader consumes
        BUG();
      }
      break;
    case BUF_STATE_EMPTY:
    case BUF_STATE_FULL:
      BUG();
  }
  
failed_waiting_to_have_space:
  endCritical(self, flags);
  return ret;
}

int circular_buffer_read(struct circular_buffer* self, int flags, void* data, size_t size, const struct timespec* abstimeout) {
  int ret = 0;
  if ((ret = startCritical(self, flags, abstimeout)) < 0)
    return ret;
  
  __block enum buf_state bufferState;
  ret = condition_wait2(&self->dataHasRead, &self->lock, CONDITION_WAIT_TIMED, abstimeout, ^bool () {
    if (circular_buffer_is_empty(self, flags & CIRCULAR_BUFFER_NO_LOCK, abstimeout))
      return true;
    
    bufferState = getBufState(self, flags, abstimeout);  
    return false;
  });
  
  if (ret < 0)
    goto failed_waiting_to_have_data;
  
  switch (bufferState) {
    case BUF_STATE_R_MORE_THAN_F: //rear > front
      if (size <= (self->r - self->f)) {
        /**  memcpy() between buffer and dequeue data
          *  start from front:f to f + dequeueSize
          *  when f + dequeueSize does not exceed r
          */
        memcpy(data, self->buf + self->f, size);
        self->f += size;
        self->f %= self->bufferSize;
      } else {
        // TODO: Do something overreading detected!
        // TODO: Overreading detected! Make it wait until new data arrived
        BUG();
      }
    case BUF_STATE_R_LESS_THAN_F:
      if (self->f + size <= self->bufferSize) {
        /**  memcpy() between buffer and dequeue data
          *  start from front:f to f + dequeueSize
          *  when f + dequeueSize is not exceed end-of-buffer   (Not Wrapping)
          *  then, dequeue only 1 section
          */
        
        memcpy(data, self->buf + self->f, size);
        self->f += size;
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
        if(size + self->f - self->bufferSize <= self->r) {
          memcpy(data, self->buf, size + self->f - self->bufferSize);
          self->f += size;
          self->f %= self->bufferSize;
        } else {
          // TODO: Overreading detected! Make it wait until new data arrived
          BUG();
        }
      }
    case BUF_STATE_FULL:
    case BUF_STATE_EMPTY:
      BUG();
  }
   
  if (self->r == self->f) {
    self->r = -1;
    self->f = -1;
  }
failed_waiting_to_have_data:
  endCritical(self, flags);
  return ret;
}
