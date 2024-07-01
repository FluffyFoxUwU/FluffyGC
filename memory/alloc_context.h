#ifndef UWU_B8777715_D44C_4AC0_9BF5_A902F9366D3A_UWU
#define UWU_B8777715_D44C_4AC0_9BF5_A902F9366D3A_UWU

#include <flup/concurrency/mutex.h>

struct alloc_tracker;
struct alloc_tracker_snapshot;
struct alloc_unit;

struct alloc_context {
  flup_mutex* contextLock;
  
  // Head and tail to allow link two list
  // of blocks one after another
  struct alloc_unit* allocListHead;
  struct alloc_unit* allocListTail;
};

struct alloc_context* alloc_context_new();
void alloc_context_free(struct alloc_tracker* self, struct alloc_context* ctx);

void alloc_context_add_block(struct alloc_context* self, struct alloc_unit* block);
void alloc_context_move_to_snapshot(struct alloc_context* self, struct alloc_tracker_snapshot* snapshot);

#endif
