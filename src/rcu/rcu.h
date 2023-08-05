#ifndef _headers_1691144010_FluffyGC_rcu
#define _headers_1691144010_FluffyGC_rcu

#include <stdint.h>

#include "concurrency/mutex.h"
#include "concurrency/condition.h"

// The heart of RCU
// everything else directly or indirectly uses this

struct rcu_head {
  _Atomic(uint64_t) readerCount;
};

struct rcu_struct {
  bool inited;
  _Atomic(uint64_t) updateBlockerCount;
  _Atomic(struct rcu_head*) current;
  
  struct mutex updaterLock;
  struct condition wakeReclaimer;
};

int rcu_init(struct rcu_struct* self);
void rcu_cleanup(struct rcu_struct* self);

struct rcu_struct* rcu_new();
void rcu_free(struct rcu_struct* self);

struct rcu_head* rcu_read(struct rcu_struct* self);

[[nodiscard("Lead to leaks")]]
struct rcu_head* rcu_read_lock(struct rcu_struct* self);
void rcu_read_unlock(struct rcu_struct* self, struct rcu_head* current);

[[nodiscard("Leads to memory leak")]]
struct rcu_head* rcu_exchange_and_synchronize(struct rcu_struct* self, struct rcu_head* data);

// This special memcpy carefully avoids the rcu_head struct
void rcu_memcpy(struct rcu_head* restrict dest, const struct rcu_head* restrict source, size_t rcuHeadOffset, size_t size);

#endif

