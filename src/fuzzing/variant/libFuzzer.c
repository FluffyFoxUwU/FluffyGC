// libFuzzer variant

#include <stddef.h>

#include "attributes.h"
#include "fuzzing/fuzzing.h"

ATTRIBUTE_USED()
int LLVMFuzzerTestOneInput(const void* data, size_t size);

ATTRIBUTE_USED()
int LLVMFuzzerTestOneInput(const void* data, size_t size) {
  return fuzzing_fuzz(data, size);
}
