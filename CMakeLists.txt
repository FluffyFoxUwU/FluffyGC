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

set(ENABLE_ASAN OFF)
set(ENABLE_MSAN OFF)
set(ENABLE_TSAN OFF)
set(ENABLE_UBSAN OFF)
set(ENABLE_XRAY OFF)

if (DEFINED CONFIG_ASAN) 
  set(ENABLE_ASAN ON)
endif()

if (DEFINED CONFIG_TSAN) 
  set(ENABLE_TSAN ON)
endif()

if (DEFINED CONFIG_MSAN) 
  set(ENABLE_MSAN ON)
endif()

if (DEFINED CONFIG_UBSAN) 
  set(ENABLE_UBSAN ON)
endif()

if (DEFINED CONFIG_LLVM_XRAY) 
  set(ENABLE_XRAY ON)
endif()

if (DEFINED CONFIG_UBSAN) 
  set(ENABLE_UBSAN ON)
endif()

add_executable(FluffyGC src/main.c
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
                        src/specials.c
                        src/premain.c
)

target_compile_options(FluffyGC PRIVATE "-g")
target_compile_options(FluffyGC PRIVATE "-O0")
target_compile_options(FluffyGC PRIVATE "-fvisibility=hidden")
target_compile_options(FluffyGC PRIVATE "-Wall")
target_compile_options(FluffyGC PRIVATE "-fblocks")
target_compile_options(FluffyGC PRIVATE "-fno-sanitize-recover=all")
target_compile_options(FluffyGC PRIVATE "-fsanitize-recover=unsigned-integer-overflow")
target_compile_options(FluffyGC PRIVATE "-fno-omit-frame-pointer")
target_compile_options(FluffyGC PRIVATE "-fno-common")
target_compile_options(FluffyGC PRIVATE "-fno-optimize-sibling-calls")

if (ENABLE_UBSAN)
  target_compile_options(FluffyGC PRIVATE "-fsanitize=undefined")
  #target_compile_options(FluffyGC PRIVATE "-fsanitize-address-use-after-return=always")
  target_compile_options(FluffyGC PRIVATE "-fsanitize=float-divide-by-zero")
  target_compile_options(FluffyGC PRIVATE "-fsanitize=implicit-conversion")
  target_compile_options(FluffyGC PRIVATE "-fsanitize=unsigned-integer-overflow")

  target_link_options(FluffyGC PRIVATE "-fsanitize=undefined")
  target_link_options(FluffyGC PRIVATE "-static-libsan")
endif()

target_compile_definitions(FluffyGC PRIVATE "PROCESSED_BY_CMAKE")

if (ENABLE_ASAN)
  target_compile_options(FluffyGC PRIVATE "-fsanitize-address-use-after-scope")
  target_compile_options(FluffyGC PRIVATE "-fsanitize=address")
  target_link_options(FluffyGC PRIVATE "-fsanitize=address")
endif()

if (ENABLE_TSAN)
  target_compile_options(FluffyGC PRIVATE "-fsanitize=thread")
  target_link_options(FluffyGC PRIVATE "-fsanitize=thread")
endif()

if (ENABLE_XRAY)
  target_compile_options(FluffyGC PRIVATE "-fxray-instrument")
  target_compile_options(FluffyGC PRIVATE "-fxray-instruction-threshold=1")
  target_link_options(FluffyGC PRIVATE "-fxray-instrument")
  target_link_options(FluffyGC PRIVATE "-fxray-instruction-threshold=1")
endif()

if (ENABLE_MSAN)
  target_compile_options(FluffyGC PRIVATE "-fsanitize-memory-track-origins")
  target_compile_options(FluffyGC PRIVATE "-fsanitize=memory")
  target_link_options(FluffyGC PRIVATE "-fsanitize=memory")
endif()

target_link_libraries(FluffyGC PRIVATE BlocksRuntime)
target_link_libraries(FluffyGC PRIVATE pthread)

target_include_directories(FluffyGC PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_include_directories(FluffyGC PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_include_directories(FluffyGC PRIVATE "${PROJECT_SOURCE_DIR}/src/collection")
target_include_directories(FluffyGC PRIVATE "${PROJECT_SOURCE_DIR}/libs")
target_include_directories(FluffyGC PRIVATE "${PROJECT_BINARY_DIR}/src")

configure_file(src/config.h src/processed_config.h)
# Abusing
configure_file(${PROJECT_SOURCE_DIR}/.config ${PROJECT_BINARY_DIR}/.config)

# Suppression files
add_custom_command(TARGET FluffyGC POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                           ${PROJECT_SOURCE_DIR}/suppressions/UBSan.supp
                           ${PROJECT_BINARY_DIR}/suppressions/UBSan.supp
                           )



