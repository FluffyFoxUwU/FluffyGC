cmake_minimum_required(VERSION 3.18)

project(FluffyGC VERSION 1.0.0
                 LANGUAGES C)

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Non clang compiler unsupported")
endif()

# C standard checks
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(scripts/process_dot_config.cmake)
include(scripts/compiler_and_link_flags.cmake)

add_library(FluffyGCObjects OBJECT
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
)

target_link_libraries(FluffyGCObjects FluffyGCCommonFlags)
target_include_directories(FluffyGCObjects PRIVATE "${PROJECT_SOURCE_DIR}/src/collection")

configure_file(src/config.h src/processed_config.h)
configure_file(${PROJECT_SOURCE_DIR}/.config ${PROJECT_BINARY_DIR}/.config)

add_library(FluffyGCShared SHARED)
target_link_libraries(FluffyGCShared FluffyGCObjects)
target_compile_options(FluffyGCShared PRIVATE "-fvisibility=hidden")

add_library(FluffyGCStatic STATIC)
target_link_libraries(FluffyGCStatic FluffyGCObjects)

# Test
add_executable(FluffyGCTest
  src/specials.c
  src/premain.c
  src/main.c
)
target_link_libraries(FluffyGCTest FluffyGCCommonFlags)
target_link_libraries(FluffyGCTest FluffyGCObjects)

# Suppression files (abusing configure)
configure_file(
  ${PROJECT_SOURCE_DIR}/suppressions/UBSan.supp
  ${PROJECT_BINARY_DIR}/suppressions/UBSan.supp
)






