#include <inttypes.h>

#include "common.h"
#include "api/mods/debug/debug.h"
#include "context.h"
#include "api/api.h"
#include "object/object.h"

bool debug_check_access(fh_object* obj, size_t offset, size_t size, enum debug_access access) {
  context_block_gc();
  struct object* internObj = atomic_load(&API_INTERN(obj)->obj);
  bool valid = offset + size < internObj->objectSize ? true : false;
  //if (!valid)
    debug_warn("Object-%" PRIu64 ": Attempt to access [%zu, %zu) which sits outside of [0, %zu) region!", internObj->movePreserve.foreverUniqueID, offset, size, internObj->objectSize);
  context_unblock_gc();
  return valid;
}
