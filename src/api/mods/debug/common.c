#include "common.h"
#include "api/mods/debug/debug.h"
#include "context.h"
#include "api/api.h"
#include "object/object.h"

bool debug_check_access(fh_object* obj, size_t offset, size_t size, enum debug_access access) {
  context_block_gc();
  struct object* internObj = atomic_load(&API_INTERN(obj)->obj);
  bool valid = offset + size < internObj->objectSize ? true : false;
  if (!valid)
    debug_warn("%s: Attempt to access [%zu, %zu) which sits outside of [0, %zu) region!", object_get_unique_name(internObj), offset, size, internObj->objectSize);
  context_unblock_gc();
  return valid;
}

const char* debug_get_unique_name(fh_object* self) {
  context_block_gc();
  struct object* obj = atomic_load(&API_INTERN(self)->obj);
  const char* name = object_get_unique_name(obj);
  context_unblock_gc();
  return name;
}
