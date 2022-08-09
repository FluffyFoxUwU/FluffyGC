include_guard()

include(scripts/process_dot_config.cmake)

add_library(CommonFlags INTERFACE)

set_property(TARGET CommonFlags
  PROPERTY POSITION_INDEPENDENT_CODE YES
)

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

target_compile_options(CommonFlags INTERFACE "-g")
target_compile_options(CommonFlags INTERFACE "-fPIC")
target_compile_options(CommonFlags INTERFACE "-O0")
target_compile_options(CommonFlags INTERFACE "-Wall")
target_compile_options(CommonFlags INTERFACE "-fblocks")
target_compile_options(CommonFlags INTERFACE "-fno-sanitize-recover=all")
target_compile_options(CommonFlags INTERFACE "-fsanitize-recover=unsigned-integer-overflow")
target_compile_options(CommonFlags INTERFACE "-fno-omit-frame-pointer")
target_compile_options(CommonFlags INTERFACE "-fno-common")
target_compile_options(CommonFlags INTERFACE "-fno-optimize-sibling-calls")
target_link_options(CommonFlags INTERFACE "-fPIC")

if (ENABLE_UBSAN)
  target_compile_options(CommonFlags INTERFACE "-fsanitize=undefined")
  #target_compile_options(CommonFlags INTERFACE "-fsanitize-address-use-after-return=always")
  target_compile_options(CommonFlags INTERFACE "-fsanitize=float-divide-by-zero")
  target_compile_options(CommonFlags INTERFACE "-fsanitize=implicit-conversion")
  target_compile_options(CommonFlags INTERFACE "-fsanitize=unsigned-integer-overflow")

  target_link_options(CommonFlags INTERFACE "-fsanitize=undefined")
  target_link_options(CommonFlags INTERFACE "-static-libsan")
endif()

target_compile_definitions(CommonFlags INTERFACE "PROCESSED_BY_CMAKE")

if (ENABLE_ASAN)
  target_compile_options(CommonFlags INTERFACE "-fsanitize-address-use-after-scope")
  target_compile_options(CommonFlags INTERFACE "-fsanitize=address")
  target_link_options(CommonFlags INTERFACE "-fsanitize=address")
endif()

if (ENABLE_TSAN)
  target_compile_options(CommonFlags INTERFACE "-fsanitize=thread")
  target_link_options(CommonFlags INTERFACE "-fsanitize=thread")
endif()

if (ENABLE_XRAY)
  target_compile_options(CommonFlags INTERFACE "-fxray-instrument")
  target_compile_options(CommonFlags INTERFACE "-fxray-instruction-threshold=1")
  target_link_options(CommonFlags INTERFACE "-fxray-instrument")
  target_link_options(CommonFlags INTERFACE "-fxray-instruction-threshold=1")
endif()

if (ENABLE_MSAN)
  target_compile_options(CommonFlags INTERFACE "-fsanitize-memory-track-origins")
  target_compile_options(CommonFlags INTERFACE "-fsanitize=memory")
  target_link_options(CommonFlags INTERFACE "-fsanitize=memory")
endif()

target_link_libraries(CommonFlags INTERFACE BlocksRuntime)
target_link_libraries(CommonFlags INTERFACE pthread)

target_include_directories(CommonFlags INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_include_directories(CommonFlags INTERFACE "${PROJECT_BINARY_DIR}/src")
target_include_directories(CommonFlags INTERFACE "${PROJECT_SOURCE_DIR}/src")

