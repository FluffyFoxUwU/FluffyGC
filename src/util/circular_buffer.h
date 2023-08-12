#ifndef _headers_1690975517_FluffyGC_circular_buffer
#define _headers_1690975517_FluffyGC_circular_buffer

// A circular buffer and does
// not allocate memory after init
//
// Reader which writes to same
// circular buffer can potentially
// cause deadlock if not used with care
//
// But writer which reads is fine as
// long as proper lock maintained
// 
// Designed for one reader and multiple writers (for use
// in logging subsystem)

// Credit to https://github.com/noahcroit/RingBuffer_and_DataFraming_C
// for majority implementation of this. Fox just adapts its
// interface to suit my needs

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "concurrency/condition.h"
#include "concurrency/mutex.h"

struct circular_buffer {
  struct mutex lock;
  struct condition dataHasRead;
  struct condition dataHasWritten;
  
  struct mutex writeLock;
  struct mutex readLock;
  
  void* allocPtr;
  void* buf;
  intptr_t r;
  intptr_t f;
  size_t bufferSize;
  
  // Bytes of ready to read data
  size_t dataInBuffer;
};

#define CIRCULAR_BUFFER_INITIALIZER(ptr, size) \
  { \
    .lock = MUTEX_INITIALIZER, \
    .dataHasRead = CONDITION_INITIALIZER, \
    .dataHasWritten = CONDITION_INITIALIZER, \
    .allocPtr = NULL, \
    .buf = (ptr), \
    .r = 0, \
    .f = 0, \
    .bufferSize = (size), \
    .dataInBuffer = 0 \
  }
#define DEFINE_CIRCULAR_BUFFER(name, ptr, size) \
  struct circular_buffer name = CIRCULAR_BUFFER_INITIALIZER(ptr, size)

#define DEFINE_CIRCULAR_BUFFER_STATIC(name, size) \
  static char  ___backingBuffer__ ## name[size]; \
  static struct circular_buffer name = CIRCULAR_BUFFER_INITIALIZER(___backingBuffer__ ## name, size)

struct circular_buffer* circular_buffer_new(size_t size);
struct circular_buffer* circular_buffer_new_from_ptr(size_t size, void* ptr);

void circular_buffer_free(struct circular_buffer* self);

// Caller has grabbed the lock necessary
#define CIRCULAR_BUFFER_NO_LOCK  0x01

// But, if gonna sleep return immediately, if this flag
// not given functions must not return -EAGAIN
// Nonblock takes over precedence to timeout
#define CIRCULAR_BUFFER_NONBLOCK 0x02
#define CIRCULAR_BUFFER_WITH_TIMEOUT 0x04

// Returns 0 on success or negative -errno
// With flags of 0 functions won't fail
// Errors:
// -ETIMEDOUT: Timed out (CIRCULAR_BUFFER_WITH_TIMEOUT)
// -EINVAL   : Invalid tv_nsec in abstimeout (CIRCULAR_BUFFER_WITH_TIMEOUT)
// -EAGAIN   : Going to block (CIRCULAR_BUFFER_NONBLOCK)
int circular_buffer_write(struct circular_buffer* self, int flags, const void* data, size_t size, const struct timespec* abstimeout);
int circular_buffer_read(struct circular_buffer* self, int flags, void* data, size_t size, const struct timespec* abstimeout);
int circular_buffer_waste(struct circular_buffer* self, int flags, size_t size, const struct timespec* abstimeout);

int circular_buffer_is_empty(struct circular_buffer* self, int flags, const struct timespec* abstimeout);
int circular_buffer_is_full(struct circular_buffer* self, int flags, const struct timespec* abstimeout);

int circular_buffer_flush(struct circular_buffer* self, int flags, const struct timespec* abstimeout);

#endif

