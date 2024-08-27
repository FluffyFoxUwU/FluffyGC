#ifndef UWU_FE9CE049_8ACB_47AE_B0EA_7CD204F8EBB1_UWU
#define UWU_FE9CE049_8ACB_47AE_B0EA_7CD204F8EBB1_UWU

/*

First idea something roughly similiar to CMS GC

The rough idea is making application
to allocate new objects as live if it
allocated during GC cycle and take
snapshot of the root set so GC can scan
it concurrently with the applications

MultiThreadedQueue<Object> mutatorMarkQueue = new MultiThreadedQueue<>()
bool cycleInProgress = false;

fun startOfGCCycle()
  // Phase 1: Preparation for GC cycle STW
  
  stop all application threads()
  cycleInProgress = true
  
  // Phase 1.a: Take snapshot of root
  take snapshot of gc root()
  
  // Phase 1.b: Make application create new objects as marked in GC perspective
  // Flip applications's perspective so it create new
  // object which is marked in GC perspective
  flip meaning of mark bit()
  resume all application threads()
  
  // Phase 2: Do marking (can be concurrent)
  for obj in rootSnapshot do
    obj.markRecursively()
  end
  
  // Phase 3: Process mutator mark queue (can be concurrent)
  // Application still can queue new thing to mark
  // covering case of mutator losing reference in such a way
  // live object did not get marked properly
  for obj in mutatorMarkQueue do
    obj.markRecursively()
  end
  
  // Phase 4: Sweeping phase (can be concurrent)
  // After scanning the GC root white objects guarantee
  // to be dead objects as newly allocated object is marked
  // to GC's perspective and there no way application
  // can get reference to unmarked objects
  free all unmarked objects()
  
  // Phase 5: Change meaning of mark bit of GC so marked objects become unmarked
  // STW to ensure mutator don't read this while cycle is not complete
  // so it did not and must not create object premarked (as there no cycle
  // going on)
  stop all application threads()
  flip GC's meaning of the mark bit()
  cycleInProgress = false
  resume all application threads()
end

// `obj` is object which was about to be overwritten
// also called when an root entry is overwritten
// (this can be also be nicely named lost of reference barrier)
fun writeBarrier(obj)
  if not cycleInProgress or obj is marked in GC's perspective then
    return
  end
  
  // So that the same object not requeued
  set obj to be marked to GC
  mutatorMarkQueue.enqueue(obj)
end

Extra notes:
1. Object which were alive which became dead during marking process
  doesn't have a good way to handle it so leave it for next GC cycle
2. Phase 2 and 3 can be concurrent (in a sense each can run
  independent of each other)
3. Phase 2, 3 and 4 each can be executed in parallel
*/

#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>

#include <flup/data_structs/buffer/circular_buffer.h>
#include <flup/concurrency/cond.h>
#include <flup/concurrency/mutex.h>
#include <flup/data_structs/dyn_array.h>
#include <flup/data_structs/buffer.h>
#include <flup/thread/thread.h>

// 8 MiB mutator mark queue size
#define GC_MUTATOR_MARK_QUEUE_SIZE (8 * 1024 * 1024)
// 16 MiB mark queue for GC only
#define GC_MARK_QUEUE_SIZE (16 * 1024 * 1024)
// 16 MiB deferred mark queue for when objects can't fit into one mark queue
#define GC_DEFERRED_MARK_QUEUE_SIZE (16 * 1024 * 1024)

#define GC_CYCLE_TIME_SAMPLE_COUNT (5)

struct generation;
struct alloc_unit;
struct thread;

struct gc_block_metadata {
  struct generation* owningGeneration;
  // Meaning changes based on flipColor on per generation state
  atomic_bool markBit;
};

enum gc_request {
  GC_NOOP,
  GC_START_CYCLE,
  GC_SHUTDOWN
};

struct gc_stats {
  uint64_t lifetimeTotalObjectCount;
  size_t lifetimeTotalObjectSize;
  
  uint64_t lifetimeTotalSweepedObjectCount;
  size_t lifetimeTotalSweepedObjectSize;
  
  uint64_t lifetimeTotalLiveObjectCount;
  size_t lifetimeLiveObjectSize;
  
  // A little note for these
  // complete count <  start count = A cycle is in progress
  // complete count == start count = GC idling
  // complete count >  start count = must not happen
  uint64_t lifetimeCyclesCompletedCount;
  uint64_t lifetimeCyclesStartCount;
  
  double lifetimeCycleTime;
  double lifetimeSTWTime;
};

struct gc_mark_state {
  struct alloc_unit* block;
  size_t fieldIndex;
};

struct gc_per_generation_state {
  flup_mutex* statsLock;
  struct gc_stats stats;
  
  atomic_size_t bytesUsedRightBeforeSweeping;
  atomic_size_t liveSetSize;
  
  struct generation* ownerGen;
  struct gc_lock_state* gcLock;
  
  flup_thread* thread;
  
  enum gc_request gcRequest;
  flup_mutex* gcRequestLock;
  flup_cond* gcRequestedCond;
  
  bool cycleWasInvoked;
  uint64_t cycleID;
  flup_mutex* cycleStatusLock;
  flup_cond* invokeCycleDoneEvent;
  
  // What mark bit value correspond to marked (
  // the meaning of the mark bit changes throughout
  // lifetime) protected by STW events
  bool mutatorMarkedBitValue;
  bool GCMarkedBitValue;
  atomic_bool cycleInProgress;
  atomic_bool markingInProgress;
  
  flup_buffer* needRemarkQueue;
  
  size_t snapshotOfRootSetSize;
  struct alloc_unit** snapshotOfRootSet;
  
  flup_circular_buffer* gcMarkQueueUwU;
  
  // For storing objects which cannot be
  // fully enqueued to the mark queue
  // so save it for later after mark queue
  // is empty
  flup_circular_buffer* deferredMarkQueue;
  
  struct gc_driver* driver;
  
  // "double" samples of cycle time in miliseconds
  struct moving_window* cycleTimeSamples;
  _Atomic(double) averageCycleTime;
};

void gc_start_cycle(struct gc_per_generation_state* self);

// Return last cycle ID before GC is called
// so caller can wait if needed
uint64_t gc_start_cycle_async(struct gc_per_generation_state* self);

// Return 0 if cycle completed or
// or -ETIMEDOUT if `absTimeout` reached and cycle hasnt completed
int gc_wait_cycle(struct gc_per_generation_state* self, uint64_t cycleID, struct timespec* absTimeout);

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen);
void gc_per_generation_state_free(struct gc_per_generation_state* self);

void gc_on_allocate(struct alloc_unit* block, struct generation* gen);
void gc_need_remark(struct alloc_unit* obj);

// These can't be nested
void gc_block(struct gc_per_generation_state* self, struct thread* blockingThread);
void gc_unblock(struct gc_per_generation_state* self, struct thread* blockingThread);

void gc_get_stats(struct gc_per_generation_state* self, struct gc_stats* stats);

void gc_perform_shutdown(struct gc_per_generation_state* self);

#endif
