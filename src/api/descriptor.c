#include "object/descriptor.h"
#include "pre_code.h"
#include "FluffyHeap.h"

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_define_descriptor(__FLUFFYHEAP_NONNULL(const char*) name, __FLUFFYHEAP_NONNULL(fh_descriptor_param*) parameter);

__FLUFFYHEAP_EXPORT __FLUFFYHEAP_NULLABLE(fh_descriptor*) fh_get_descriptor(__FLUFFYHEAP_NONNULL(const char*) name, bool dontInvokeLoader);
__FLUFFYHEAP_EXPORT void fh_release_descriptor(__FLUFFYHEAP_NULLABLE(fh_descriptor*) desc) {
  descriptor_release(INTERN(desc));
}
