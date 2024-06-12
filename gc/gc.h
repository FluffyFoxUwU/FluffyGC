#ifndef UWU_FE9CE049_8ACB_47AE_B0EA_7CD204F8EBB1_UWU
#define UWU_FE9CE049_8ACB_47AE_B0EA_7CD204F8EBB1_UWU

/*

The rough idea is making application
to allocate new objects as live if it
allocated during GC cycle and take
snapshot of the root set so GC can scan
it concurrently with the applications

fun startOfGCCycle()
  stop all application threads()
  take snapshot of gc root()
  // Flip applications's perspective so it create new
  // object which colored black to GC's perspective
  flip application's perspective of the colors()
  resume all application threads()
  
  mark the GC root recursively()
  
  // After scanning the GC root white objects guarantee
  // to be dead objects as newly allocated object is colored
  // black to GC's perspective and there no way application
  // can get reference to white objects
  free all white objects()
  
  // Flip GC's perspective of the colors so on next
  // color currently black objects is now white
  flip GC's perspective of the colors()
end

Extra notes:
1. Object which were alive which became dead during marking process
  doesn't have a good way to handle it so leave it for next GC cycle
2. Black and white object can be represented as one mark bit (where
  marked would be black and unmarked would be white)
*/

#include <stdatomic.h>

#include <flup/data_structs/buffer/circular_buffer.h>

// 8 MiB mark queue size
#define GC_MARK_QUEUE_SIZE (8 * 1024 * 1024)

struct generation;
struct arena_block;

struct gc_block_metadata {
  // Meaning changes based on flipColor on per generation state
  bool markBit;
};

struct gc_per_generation_state {
  struct generation* ownerGen;
  atomic_bool flipColor;
  
  flup_circular_buffer* markQueue;
};

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen);
void gc_per_generation_state_free(struct gc_per_generation_state* self);

void gc_on_allocate(struct arena_block* block, struct generation* gen);

#endif
