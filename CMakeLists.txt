cmake_minimum_required(VERSION 3.18)

# Source of truth for version is 
# NOT HERE its in Kconfig file
project(FluffyGC VERSION 1.0.0
                 LANGUAGES C)

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Non clang compiler unsupported")
endif()

# C standard checks
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

##################################
## Dont touch anything but this ##
##################################
set(FluffyGC_SOURCES
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

set(FluffyGC_PUBLIC_HEADERS
  include/FluffyGC/common.h
  include/FluffyGC/v1.h
)

set(FluffyGC_TEST_SOURCES
  src/specials.c
  src/premain.c
  src/main.c
)
##################################
  
# Suppression files (abusing configure)
configure_file(
  ${PROJECT_SOURCE_DIR}/suppressions/UBSan.supp 
  ${PROJECT_BINARY_DIR}/suppressions/UBSan.supp
)

##################################
include(scripts/process_dot_config.cmake)
include(scripts/compiler_and_link_flags.cmake)

configure_file(src/config.h src/processed_config.h)
configure_file(${PROJECT_SOURCE_DIR}/.config ${PROJECT_BINARY_DIR}/.config)

add_library(FluffyGC ${FluffyGC_SOURCES})

## Deps here ##
target_link_libraries(FluffyGC ProjectCommonFlags)
###############

add_executable(FluffyGCTest ${FluffyGC_TEST_SOURCES})
target_link_libraries(FluffyGCTest ProjectCommonFlags)
target_link_libraries(FluffyGCTest FluffyGC)

# Installations
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

# Install library headers
install(
  FILES ${FluffyGC_PUBLIC_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/FluffyGC
)

# Install library
install(
  TARGETS FluffyGC
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/
)

# Install pkgconfig file
set(target FluffyGC)
configure_file(package.pc.in
  ${CMAKE_BINARY_DIR}/FluffyGC.pc @ONLY)
install(
  FILES ${CMAKE_BINARY_DIR}/FluffyGC.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/
)































