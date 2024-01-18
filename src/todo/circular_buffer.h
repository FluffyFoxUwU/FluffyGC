#ifndef _headers_1690280980_FluffyGC_circular_buffer
#define _headers_1690280980_FluffyGC_circular_buffer

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

#include <stdint.h>
#include <time.h>

#include "concurrency/condition.h"
#include "concurrency/mutex.h"

struct circular_buffer {
  struct mutex lock;
  struct condition dataHasRead;
  struct condition dataHasWritten;
  
  void* allocPtr;
  void* pool;
  
  size_t capacity;
  size_t dataReady;
  
  uintptr_t readHeadOffset;
  uintptr_t writeHeadOffset;
};

struct circular_buffer* circular_buffer_new(size_t size);
struct circular_buffer* circular_buffer_new_from_ptr(size_t size, void* ptr);

void circular_buffer_free(struct circular_buffer* self);

#define CIRCULAR_BUFFER_NO_LOCK 0x01

// Returns:
// -ETIMEDOUT: Timed out waiting for free space (may be blocked longer than
//             this if cause of block not due out of space)
// -EINVAL   : Invalid timeout (follows POSIX's condition for the spec "The abstime argument specified a nanosecond value less than zero or greater than or equal to 1000 million.")
// -EOVERFLOW: (Writer only) Too large write request
int circular_buffer_write(struct circular_buffer* self, const void* data, size_t size, int flags, const struct timespec* timeout);
int circular_buffer_read(struct circular_buffer* self, void* data, size_t size, int flags, const struct timespec* timeout);

#endif

