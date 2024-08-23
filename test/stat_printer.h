#ifndef UWU_CCBDE72B_4973_4358_AC72_F8F3DF639634_UWU
#define UWU_CCBDE72B_4973_4358_AC72_F8F3DF639634_UWU

#include "heap/heap.h"
#include <flup/concurrency/cond.h>
#include <flup/concurrency/mutex.h>
#include <flup/thread/thread.h>
#include <stdint.h>

enum stat_printer_status {
  STAT_PRINTER_STATUS_STOPPED,
  STAT_PRINTER_STATUS_STARTED,
  STAT_PRINTER_STATUS_SHUTDOWNED
};

enum stat_printer_request {
  STAT_PRINTER_REQUEST_STOP,
  STAT_PRINTER_REQUEST_START,
  STAT_PRINTER_REQUEST_SHUTDOWN
};

struct stat_printer {
  struct heap* heap;
  
  // Lock and cond for printer status
  flup_mutex* statusLock;
  flup_cond* statusUpdatedEvent;
  enum stat_printer_status status;
  uint64_t latestExecutedReqID;
  
  // Lock and cond for request
  flup_mutex* requestLock;
  flup_cond* requestNeededEvent;
  enum stat_printer_request request;
  uint64_t reqCount;
  
  flup_thread* printerThread;
};

struct stat_printer* stat_printer_new(struct heap* heap);
void stat_printer_start(struct stat_printer* self);
void stat_printer_stop(struct stat_printer* self);
void stat_printer_free(struct stat_printer* self);

#endif
