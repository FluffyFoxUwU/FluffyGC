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

fun startOfGCCycle()
  // Take root snapshot (one of two STW pauses)
  
  stop all application threads()
  take snapshot of gc root()
  // Flip applications's perspective so it create new
  // object which is marked to GC perspective
  flip meaning of mark bit()
  resume all application threads()
  
  // Concurrent marking (works on root)
  for obj in rootSnapshot do
    obj.markRecursively()
  end
  
  // Remark phase (concurrent)
  
  // Process mark queue while application still queue-ing until
  // the queue empty
  for obj in mutatorMarkQueue do
    obj.markRecursively()
  end
  
  // Sweeping phase (concurrent)
  // After scanning the GC root white objects guarantee
  // to be dead objects as newly allocated object is marked
  // to GC's perspective and there no way application
  // can get reference to unmarked objects
  free all white objects()
  
  flip GC's meaning of the mark bit()
end

// `obj` is object which was about to be overwritten
// also called when an root entry is overwritten
fun writeBarrier(obj)
  if obj is marked to GC then
    return
  end
  
  // So that the same object not requeued
  set obj to be marked to GC
  mutatorMarkQueue.enqueue(obj)
end

Extra notes:
1. Object which were alive which became dead during marking process
  doesn't have a good way to handle it so leave it for next GC cycle
*/

#include <stdatomic.h>

#include <flup/data_structs/buffer.h>

// 8 MiB mutator mark queue size
#define GC_MARK_QUEUE_SIZE (8 * 1024 * 1024)

struct generation;
struct arena_block;

struct gc_block_metadata {
  struct generation* owningGeneration;
  // Meaning changes based on flipColor on per generation state
  atomic_bool markBit;
};

struct gc_per_generation_state {
  struct generation* ownerGen;
  
  // What mark bit value correspond to marked (
  // the meaning of the mark bit changes throughout
  // lifetime)
  bool mutatorMarkedBitValue;
  bool GCMarkedBitValue;
  
  flup_buffer* mutatorMarkQueue;
};

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen);
void gc_per_generation_state_free(struct gc_per_generation_state* self);

void gc_on_allocate(struct arena_block* block, struct generation* gen);
void gc_on_write(struct arena_block* objectWhichIsGoingToBeOverwritten);

#endif
