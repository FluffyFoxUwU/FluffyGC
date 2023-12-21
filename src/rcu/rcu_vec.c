#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rcu_vec.h"
#include "vec.h"

static int copyFunc(const void* _src, void* _dest) {
  int ret = 0;
  const struct rcu_vec_head* src = _src;
  struct rcu_vec_head* dest = _dest;
  memcpy(dest, src, sizeof(*dest));
  
  const vec_t(void*)* proxyArrayForSrc = (void*) ((char*) _src + src->arrayOffset);
  vec_t(void*)* proxyArrayForDest = (void*) ((char*) _dest + dest->arrayOffset);
  memcpy(proxyArrayForDest, proxyArrayForSrc, sizeof(*proxyArrayForSrc));
  
  if (proxyArrayForSrc->data == NULL)
    goto no_copy_data;
  
  // Create copy of the data
  proxyArrayForDest->data = calloc(proxyArrayForSrc->capacity, src->elementSize);
  if (!proxyArrayForDest) {
    ret = -ENOMEM;
    goto alloc_error;
  }
  
  memcpy(proxyArrayForDest->data, proxyArrayForSrc->data, proxyArrayForSrc->capacity * src->elementSize);
alloc_error:
no_copy_data:
  return ret;
}

static void cleanupFunc(void* _data) {
  const struct rcu_vec_head* data = _data;
  vec_t(void*)* arr = (void*) ((char*) _data + data->arrayOffset);
  vec_deinit(arr);
}

struct rcu_generic_ops rcu_vec_t_ops = {
  .copy = copyFunc,
  .cleanup = cleanupFunc
};
