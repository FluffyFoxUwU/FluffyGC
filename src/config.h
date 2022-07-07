#ifdef PROCESSED_BY_CMAKE
# ifndef header_1655953188_config_h_in_loaded
# define header_1655953188_config_h_in_loaded
#  include "processed_config.h"
# endif
#endif

#ifndef header_1655953188_config_h_in
#define header_1655953188_config_h_in

#include "compiler_config.h"

#ifdef PROCESSED_BY_CMAKE
# define FLUFFYGC_VERSION_MAJOR @FluffyGC_VERSION_MAJOR@
# define FLUFFYGC_VERSION_MINOR @FluffyGC_VERSION_MINOR@
# define FLUFFYGC_VERSION_PATCH @FluffyGC_VERSION_PATCH@
#else
# define FLUFFYGC_VERSION_MAJOR 0
# define FLUFFYGC_VERSION_MINOR 0
# define FLUFFYGC_VERSION_PATCH 0
#endif

#define FLUFFYGC_REGION_ENABLE_POISONING (1)

#define FLUFFYGC_REGION_CELL_SIZE (64)
#define FLUFFYGC_REGION_CELL_ALIGNMENT FLUFFYGC_REGION_CELL_SIZE
#define FLUFFYGC_REGION_POISON_GRANULARITY (8)


// Each bucket contain 64 cells
#define FLUFFYGC_HEAP_CARD_TABLE_PER_BUCKET_SIZE (64)

// Size to increase when the thread list is full
#define FLUFFYGC_HEAP_THREAD_LIST_STEP_SIZE (16)

// Max retries count before giving up
#define FLUFFYGC_HEAP_MAX_YOUNG_ALLOC_RETRIES (5)
#define FLUFFYGC_HEAP_MAX_OLD_ALLOC_RETRIES (2)

#endif

