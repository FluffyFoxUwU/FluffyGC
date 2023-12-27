#ifndef header_1703580680_6f38372b_9888_4dc6_8194_1f7c03cc0c76_heap_h
#define header_1703580680_6f38372b_9888_4dc6_8194_1f7c03cc0c76_heap_h

#include "config.h"

#if IS_ENABLED(CONFIG_HEAP_FLAVOUR_SIMPLE)
# include "heap_simple/heap.h" // IWYU pragma: export
#endif

#if IS_ENABLED(CONFIG_HEAP_FLAVOUR_MALLOC)
# include "heap_malloc/heap.h" // IWYU pragma: export
#endif

#endif

