# Build config

set(BUILD_PROJECT_NAME "FluffyGC")

# Sources which common between exe and library
set(BUILD_SOURCES
  src/util.c
  src/descriptor.c
  src/context.c
  src/profiler.c
  src/thread_pool.c
  src/object.c
  src/bitops.c
  src/soc.c
  src/heap.c
  src/heap_free_block_searchers.c
  src/free_list_sorter.c
  
  deps/list/list_node.c
  deps/list/list.c
  deps/list/list_iterator.c  
  deps/templated-hashmap/hashmap.c  
  deps/vec/vec.c
)

set(BUILD_INCLUDE_DIRS
  deps/list/
  deps/templated-hashmap/
  deps/vec/
)

# Note that exe does not represent Windows' 
# exe its just short hand of executable 
#
# Note:
# Still creates executable even building library. 
# This would contain test codes if project is 
# library. The executable directly links to the 
# library objects instead through shared library
# these not built if CONFIG_DISABLE_MAIN set
set(BUILD_EXE_SOURCES
  src/specials.c
  src/premain.c
  src/main.c
)

# Public header to be exported
# If this a library
set(BUILD_PUBLIC_HEADERS
  include/dummy.h
)

set(BUILD_PROTOBUF_FILES
)

set(BUILD_CFLAGS "")
set(BUILD_LDFLAGS "")

# AddPkgConfigLib is in ./buildsystem/CMakeLists.txt
macro(AddDependencies)
  # Example
  # AddPkgConfigLib(FluffyGC FluffyGC>=1.0.0)
endmacro()

macro(PreConfigurationLoad)
  # Do pre config stuffs
endmacro()

macro(PostConfigurationLoad)
  # Do post config stuffs
  # like deciding whether to include or not include some files
  
  if (DEFINED CONFIG_FUZZER_LIBFUZZER)
    string(APPEND BUILD_CFLAGS " -fsanitize=fuzzer")
    string(APPEND BUILD_LDFLAGS " -fsanitize=fuzzer")
  endif()
  
  if (DEFINED CONFIG_MAIN_DISABLED)
    set(BUILD_EXE_SOURCES "")
  endif()
  
  if (DEFINED CONFIG_FUZZER_LIBFUZZER)
    list(APPEND BUILD_EXE_SOURCES "./src/fuzzing/variant/libFuzzer.c")
  endif()
  
  if (DEFINED CONFIG_FUZZ_SOC)
    list(APPEND BUILD_EXE_SOURCES "./src/fuzzing/fuzzing_soc.c")
  endif()
  
  if (DEFINED CONFIG_FUZZ_HEAP)
    list(APPEND BUILD_EXE_SOURCES "./src/fuzzing/fuzzing_heap.c")
  endif()
endmacro()

