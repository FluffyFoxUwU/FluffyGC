// libFuzzer variant

#include <stddef.h>
#include <stdint.h>

#include "compiler_config.h"
#include "fuzzing/fuzzing.h"

ATTRIBUTE_USED()
int LLVMFuzzerTestOneInput(const void* data, size_t size) {
  return fuzzing_fuzz(data, size);
}
