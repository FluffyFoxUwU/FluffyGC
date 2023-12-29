#ifndef header_1703732470_f8d76247_7d1b_415b_9355_9177f4e05d46_lazy_init_rwlock_h
#define header_1703732470_f8d76247_7d1b_415b_9355_9177f4e05d46_lazy_init_rwlock_h

// Its initialized only after
// writer attempt to use

#include <stdatomic.h>

#include "concurrency/condition.h"
#include "concurrency/rwlock.h"

struct rwlock_lazy {
  atomic_bool inited;
  atomic_int_least32_t readCount;

  atomic_bool condInited;
  struct condition readerGone;
  
  atomic_bool lazyInited;
  struct rwlock rwlock;
};

/*

void read_lock() {
  if (!lazyInited)
    readCount++;
  else
    rwlock.readLock();
}

void read_unlock() {
  if (readCount == 0 && lazyInited)
    rwlock.readUnlock();
  else if (readCount == 0 && condInited)
    readerGone.wake();
  else
    readCount--;
}

*/

#endif

