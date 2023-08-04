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
  
  void* buf;
  uintptr_t r;
  uintptr_t f;
  uintptr_t bufferSize;
};

struct circular_buffer* circular_buffer_new(size_t size);
struct circular_buffer* circular_buffer_new_from_ptr(size_t size, void* ptr);

void circular_buffer_free(struct circular_buffer* self);

// Caller has grabbed the lock necessary
#define CIRCULAR_BUFFER_NO_LOCK  0x01

// But, if gonna sleep return immediately, if this flag
// not given functions must not return -EAGAIN
#define CIRCULAR_BUFFER_NONBLOCK 0x02
#define CIRCULAR_BUFFER_WITH_TIMEOUT 0x04

// Returns:
// -ETIMEDOUT: Timed out waiting for free space
// -EINVAL   : Invalid timeout (follows POSIX's condition for the spec "The abstime argument specified a nanosecond value less than zero or greater than or equal to 1000 million.")
// -EOVERFLOW: (Writer only) Too large write request
// -EAGAIN   : Going to sleep
int circular_buffer_write(struct circular_buffer* self, int flags, const void* data, size_t size, const struct timespec* abstimeout);
int circular_buffer_read(struct circular_buffer* self, int flags, void* data, size_t size, const struct timespec* abstimeout);

// Same as circular_buffer_read but do not reads just waste
// `size` bytes of data
int circular_buffer_waste(struct circular_buffer* self, int flags, size_t size, const struct timespec* abstimeout);

int circular_buffer_is_empty(struct circular_buffer* self, int flags, const struct timespec* abstimeout);
int circular_buffer_is_full(struct circular_buffer* self, int flags, const struct timespec* abstimeout);

#endif

