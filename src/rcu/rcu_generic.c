#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "rcu_generic.h"
#include "rcu/rcu.h"
#include "util/util.h"

#define GENERIC_CONTAINER(x) container_of(x, struct rcu_generic_container, rcuHead)

int rcu_generic_init(struct rcu_generic* self, struct rcu_generic_ops* ops) {
  self->ops = ops;
  return rcu_init(&self->rcu);
}

void rcu_generic_cleanup(struct rcu_generic* self) {
  rcu_generic_write(self, NULL);
  rcu_cleanup(&self->rcu);
}

// Get readonly version
struct rcu_generic_container* rcu_generic_get_readonly(struct rcu_generic* self) {
  return GENERIC_CONTAINER(rcu_read(&self->rcu));
}

// Writing
void rcu_generic_write(struct rcu_generic* self, struct rcu_generic_container* new) {
  struct rcu_head* head = new ? &new->rcuHead : NULL;
  struct rcu_head* old = rcu_exchange_and_synchronize(&self->rcu, head);
  if (!old)
    return;
  
  struct rcu_generic_container* oldContainer = GENERIC_CONTAINER(old);
  
  if (self->ops && self->ops->cleanup)
    self->ops->cleanup(oldContainer->data.ptr);
  rcu_generic_dealloc_container(oldContainer);
}

struct rcu_generic_container* rcu_generic_exchange(struct rcu_generic* self, struct rcu_generic_container* new) {
  struct rcu_head* old = rcu_exchange_and_synchronize(&self->rcu, &new->rcuHead);
  if (!old)
    return NULL;
  return GENERIC_CONTAINER(old);
}

// Get writeable copy
struct rcu_generic_container* rcu_generic_copy(struct rcu_generic* self) {
  int ret = 0;
  struct rcu_head* currentRcu; 
  struct rcu_generic_container* currentContainer;
  size_t size;
  
  currentRcu = rcu_read_lock(&self->rcu);
  currentContainer = GENERIC_CONTAINER(rcu_read(&self->rcu));
  size = currentContainer->size;
  rcu_read_unlock(&self->rcu, currentRcu);
  
  struct rcu_generic_container* copy = rcu_generic_alloc_container(size);
  if (!copy)
    goto failure;
  
  currentRcu = rcu_read_lock(&self->rcu);
  currentContainer = GENERIC_CONTAINER(rcu_read(&self->rcu));
  // Do the copy
  if (self->ops && self->ops->copy)
    ret = self->ops->copy(currentContainer->data.ptr, copy->data.ptr);
  else
    memcpy(copy->data.ptr, currentContainer->data.ptr, currentContainer->size);
  rcu_read_unlock(&self->rcu, currentRcu);

failure:
  // If copy operation failed, deallocate
  if (ret < 0 && copy)
    rcu_generic_dealloc_container(copy);
  return copy;
}

// Container management
struct rcu_generic_container* rcu_generic_alloc_container(size_t size) {
  size_t minAlignment = alignof(max_align_t);
  struct rcu_generic_container* new = malloc(sizeof(*new) + size + minAlignment);
  if (!new)
    return NULL;
  
  new->data.ptr = (PTR_ALIGN(&new->dataArray, minAlignment));
  new->size = size;
  return new;
}

void rcu_generic_dealloc_container(struct rcu_generic_container* container) {
  free(container);
}
