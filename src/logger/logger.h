#ifndef _headers_1690279182_FluffyGC_logger
#define _headers_1690279182_FluffyGC_logger

#include <pthread.h>
#include <stdint.h>

// Atomic logger
// log is overwritten if reader
// can't keep up. Reads is not
// reentrant

enum logger_loglevel {
  LOG_EMERGENCY,
  LOG_ALERT,
  LOG_CRITICAL,
  LOG_ERROR,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG
};

#define LOG_DEFAULT LOG_INFO

struct logger {
  _Atomic(uint64_t) sequenceNumber;
};

#endif

