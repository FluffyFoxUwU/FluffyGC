#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "concurrency/rwulock.h"
#include "context.h"
#include "gc.h"
#include "itc/channel.h"
#include "managed_heap.h"
#include "bug.h"
#include "config.h"

#include "gc/nop/nop.h"
#include "gc/serial/serial.h"
#include "memory/heap.h"
#include "util/list_head.h"
#include "util/util.h"
#include "vec.h"

thread_local struct gc_struct* gc_current = NULL;

#define SELECT_GC_ENTRY(name, prefix, ret, func, ...) \
  case name: \
    ret = prefix ## func(__VA_ARGS__); \
    break; \

#define SELECT_GC(algo, ret, func, ...) \
  switch (algo) { \
    case GC_UNKNOWN: \
      BUG(); \
    SELECT_GC_ENTRY(GC_NOP_GC, gc_nop_, ret, func, __VA_ARGS__) \
    SELECT_GC_ENTRY(GC_SERIAL_GC, gc_serial_, ret, func, __VA_ARGS__) \
  }

static void processMessage(struct channel_message* msg, struct channel_message* response) {
  enum gc_operation op = msg->identifier;
  
  switch (op) {
    case GC_OP_INIT: {
      struct gc_init_data* data = msg->args[0].pData;
      gc_current = data->gc;
      managed_heap_current = data->heap;
      SELECT_GC(data->algo, gc_current->ops, new, data->gcFlags);
      
      response->identifier = gc_current->ops ? GC_STATUS_OK : GC_STATUS_FAILED;
      break;
    }
    case GC_OP_SHUTDOWN: {
      if (gc_current->ops)
        gc_current->ops->free(gc_current->ops);
      response->identifier = GC_STATUS_OK;
      break;
    }
    case GC_OP_COLLECT: {
      gc_current->stat.reclaimedBytes += gc_current->ops->collect(msg->args[0].pData);
      response->identifier = GC_STATUS_OK;
      break;
    }
    case GC_OP_PING: {
      response->identifier = GC_STATUS_OK;
      break;
    }
  }
}

static void* mainGCThread(void* _self) {
  struct gc_struct* self = _self;
  struct channel_message msg = {};
  struct channel_message resp = {};
  
  util_set_thread_name("FlGC: GC Main");
  
  while (true) {
    channel_recv(self->commandChannel, &msg);
    atomic_thread_fence(memory_order_seq_cst);
    processMessage(&msg, &resp);
    resp.cookie = msg.cookie;
    atomic_thread_fence(memory_order_seq_cst);
    channel_send(self->responseChannel, &resp);
    
    if (msg.identifier == GC_OP_SHUTDOWN)
      break;
  }
  
  return NULL;
}

static enum gc_status callGC(struct gc_struct* self, struct channel_message* msg, struct channel_message* respPtr) {
  struct channel_message resp = {};
  msg->cookie = (uintptr_t) msg;
  
  if (!IS_ENABLED(CONFIG_GC_SEPERATE_THREAD)) {
    processMessage(msg, &resp);
    goto done_call;
  }
  
  atomic_thread_fence(memory_order_seq_cst);
  channel_send(self->commandChannel, msg);
  
  while (true) {
    channel_recv(self->responseChannel, &resp);
    atomic_thread_fence(memory_order_seq_cst);
    if (resp.cookie == msg->cookie)
      break;
    
    // This not our response, send back to channel
    atomic_thread_fence(memory_order_seq_cst);
    channel_send(self->responseChannel, &resp);
  }
  
done_call:
  if (respPtr)
    *respPtr = resp;
  return resp.identifier;
}

struct gc_struct* gc_new(enum gc_algorithm algo, gc_flags gcFlags) {
  struct gc_struct* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_struct) {};
  
  self->commandChannel = channel_new();
  self->responseChannel = channel_new();
  if (!self->commandChannel || !self->responseChannel || rwulock_init(&self->gcLock) < 0)
    goto failure;
  
  if (IS_ENABLED(CONFIG_GC_SEPERATE_THREAD)) {
    if (pthread_create(&self->mainThread, NULL, mainGCThread, self) != 0)
      goto failure;
    self->mainThreadIsUpAndSqueaking = true;
  }
  
  struct gc_init_data initData = {
    .algo = algo,
    .gcFlags = gcFlags,
    .heap = managed_heap_current,
    .gc = self
  };
  
  struct channel_message msg = {
    .identifier = GC_OP_INIT,
    .cookie = 0,
    .args = {{.pData = &initData}}
  };
  if (callGC(self, &msg, NULL) != GC_STATUS_OK)
    goto failure;
  
  return self;
failure:
  gc_free(self);
  return NULL;
}

int gc_generation_count(enum gc_algorithm algo, gc_flags gcFlags) {
  int ret;
  SELECT_GC(algo, ret, generation_count, gcFlags);
  return ret;
}

void gc_start(struct gc_struct* self, struct generation* gen) {
  callGC(self, &CHANNEL_MESSAGE(
    .identifier = GC_OP_COLLECT,
    .args = {{.pData = gen}}
  ), NULL);
}

void gc_free(struct gc_struct* self) {
  if (!self)
    return;
  
  // Call shutdown if main thread is up or seperate thread disabled
  if (self->mainThreadIsUpAndSqueaking || !IS_ENABLED(CONFIG_GC_SEPERATE_THREAD))
    callGC(self, &CHANNEL_MESSAGE(GC_OP_SHUTDOWN), NULL);
  
  if (self->mainThreadIsUpAndSqueaking && IS_ENABLED(CONFIG_GC_SEPERATE_THREAD))
    pthread_join(self->mainThread, NULL);
  
  rwulock_cleanup(&self->gcLock);
  channel_free(self->commandChannel);
  channel_free(self->responseChannel);
  free(self);
}

bool gc_use_fast_on_gen(enum gc_algorithm algo, gc_flags gcFlags, int genID) {
  bool ret;
  SELECT_GC(algo, ret, use_fast_on_gen, gcFlags, genID);
  return ret;
}

void gc_downgrade_from_gc_mode(struct gc_struct* self) {
  rwulock_downgrade(&self->gcLock);
}

bool gc_upgrade_to_gc_mode(struct gc_struct* self) {
  return rwulock_try_upgrade(&self->gcLock);
}

void gc_for_each_root_entry(struct gc_struct* self, void (^iterator)(struct root_ref*)) {
  for (int i = 0; i < CONTEXT_STATE_COUNT; i++) {
    struct list_head* currentContext;
    list_for_each(currentContext, &managed_heap_current->contextStates[i]) {
      struct list_head* current;
      list_for_each(current, &list_entry(currentContext, struct context, list)->root)
        iterator(list_entry(current, struct root_ref, node));
    }
  }
}
