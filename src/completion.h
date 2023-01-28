#ifndef _headers_1674914743_FluffyGC_completion
#define _headers_1674914743_FluffyGC_completion

#include <stdbool.h>

#include "event.h"

struct completion {
  struct event onComplete;
  int done;
};

#endif

