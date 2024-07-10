#ifndef UWU_BD8E93FB_F342_4108_B5D4_16458F0CCC83_UWU
#define UWU_BD8E93FB_F342_4108_B5D4_16458F0CCC83_UWU

// A generic interface, to do block and unblock GC.
// Can be implemented as pthread_rwlock's for portable fallback
//
// All implementer should be prefer GC (i.e. when GC need exclusive
// access then threads cant block it while it waiting)

struct gc_lock_per_thread_data;
struct gc_lock_state;

struct gc_lock_state* gc_lock_new();
void gc_lock_free(struct gc_lock_state* self);

struct gc_lock_per_thread_data* gc_lock_new_thread(struct gc_lock_state* self);
void gc_lock_free_thread(struct gc_lock_state* self, struct gc_lock_per_thread_data* thread);

// The operations
void gc_lock_enter_gc_exclusive(struct gc_lock_state* self);
void gc_lock_exit_gc_exclusive(struct gc_lock_state* self);

void gc_lock_block_gc(struct gc_lock_state* self, struct gc_lock_per_thread_data* thread);
void gc_lock_unblock_gc(struct gc_lock_state* self, struct gc_lock_per_thread_data* thread);

#endif
