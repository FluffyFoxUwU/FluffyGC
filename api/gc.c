#include <flup/util/min_max.h>
#include <stddef.h>

#include <FluffyGC/FluffyGC.h>
#include <FluffyGC/gc.h>
#include <FluffyGC/export.h>

static const struct fluffygc_gc algorithmns[] = {
  {
    .version = 0x1'00'00,
    .name = "Foxie's Mostly Concurrent GC",
    .id = "1c442686-b9e2-44c1-ab66-32b924eaa210-foxie-concurrent-gc",
    .description = "Its a low pause GC based on ZGC design and bits of Shenandoah plus mimalloc LD_PRELOAD'ed (for optimum performance)"
  }
};

FLUFFYGC_EXPORT
size_t fluffygc_gc_get_algorithmns(struct fluffygc_gc* output, size_t maxCount) {
  size_t algosCount = sizeof(algorithmns) / sizeof(*algorithmns);
  if (!output || maxCount == 0)
    goto only_size_requested;
  
  size_t copyCount = flup_min(algosCount, maxCount);
  for (size_t i = 0; i < copyCount; i++)
    output[i] = algorithmns[i];
only_size_requested:
  return algosCount;
}

