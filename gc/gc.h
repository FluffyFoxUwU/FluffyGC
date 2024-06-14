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

#include <flup/data_structs/dyn_array.h>
#include <flup/data_structs/buffer.h>
#include <flup/concurrency/rwlock.h>

// 8 MiB mutator mark queue size
#define GC_MARK_QUEUE_SIZE (8 * 1024 * 1024)

struct generation;
struct arena_block;

struct gc_block_metadata {
  struct generation* owningGeneration;
  atomic_bool isValid;
  // Meaning changes based on flipColor on per generation state
  atomic_bool markBit;
};

struct gc_per_generation_state {
  struct generation* ownerGen;
  flup_rwlock* gcLock;
  
  // What mark bit value correspond to marked (
  // the meaning of the mark bit changes throughout
  // lifetime) protected by STW events
  bool mutatorMarkedBitValue;
  bool GCMarkedBitValue;
  bool cycleInProgress;
  
  flup_buffer* mutatorMarkQueue;
  flup_dyn_array* snapshotOfRootSet;
};

void gc_start_cycle(struct gc_per_generation_state* self);

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen);
void gc_per_generation_state_free(struct gc_per_generation_state* self);

void gc_on_allocate(struct arena_block* block, struct generation* gen);
void gc_on_reference_lost(struct arena_block* objectWhichIsGoingToBeOverwritten);

// These can't be nested
void gc_block(struct gc_per_generation_state* self);
void gc_unblock(struct gc_per_generation_state* self);

#endif
