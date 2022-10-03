# Build config

set(BUILD_PROJECT_NAME "FluffyGC")

# We're making library
set(BUILD_IS_LIBRARY YES)

# If we want make libary and
# executable project
set(BUILD_INSTALL_EXECUTABLE NO)

# Sources which common between exe and library
set(BUILD_SOURCES
  src/region.c
  src/asan_stuff.c
  src/util.c
  src/descriptor.c
  src/heap.c
  src/root.c
  src/thread.c
  src/root_iterator.c
  src/gc/gc.c
  src/gc/young_collector.c
  src/gc/old_collector.c
  src/gc/full_collector.c
  src/gc/marker.c
  src/collection/hashmap.c
  src/profiler.c
  src/collection/list.c
  src/collection/list_node.c
  src/collection/list_iterator.c
  src/gc/cardtable_iterator.c
  src/api_layer/v1.c
  src/reference_iterator.c
  src/thread_pool.c
  src/gc/common.c
  src/gc/parallel_marker.c
  src/gc/parallel_heap_iterator.c
  src/userfaultfd.c
)

# Note that exe does not represent Windows' 
# exe its just short hand of executable 
#
# Note:
# Still creates executable even building library. 
# This would contain test codes if project is 
# library. The executable directly links to the 
# library objects instead through shared library
set(BUILD_EXE_SOURCES
  src/specials.c
  src/premain.c
  src/main.c
)

# Public header to be exported
# If this a library
set(BUILD_PUBLIC_HEADERS
  include/FluffyGC/common.h
  include/FluffyGC/v1.h
)

# AddPkgConfigLib is in ./buildsystem/CMakeLists.txt
macro(AddDependencies)
  # Example
  # AddPkgConfigLib(FluffyGC FluffyGC>=1.0.0)
endmacro()


