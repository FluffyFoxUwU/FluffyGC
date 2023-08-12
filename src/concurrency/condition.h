#ifndef _headers_1674913199_FluffyGC_condition
#define _headers_1674913199_FluffyGC_condition

#include <pthread.h>
#include <stdbool.h>

struct mutex;
struct condition {
  pthread_cond_t cond;
  bool inited;
};

#define CONDITION_INITIALIZER \
  {.cond = PTHREAD_COND_INITIALIZER, .inited = true}

typedef bool (^condition_checker)();

int condition_init(struct condition* self);
void condition_cleanup(struct condition* self);

#define CONDITION_WAIT_TIMED      0x01
#define CONDITION_WAIT_NO_CHECKER 0x02

// the checker acts like
// while (checker())
//   <whatever waiting function>
void condition_wake(struct condition* self);
void condition_wake_all(struct condition* self);
void condition_wait(struct condition* self, struct mutex* mutex, condition_checker checker);

// If flags == 0 it acts like condition_wait, because it is void. 
// call with flags == 0 never return errors (always 0)
// Errors:
// -EINVAL   : Invalid absolute time
// -ETIMEDOUT: Timed out
int condition_wait2(struct condition* self, struct mutex* mutex, int flags, const struct timespec* abstime, condition_checker checker);

#endif

