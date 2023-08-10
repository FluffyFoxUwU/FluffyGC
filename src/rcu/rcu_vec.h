#ifndef _headers_1691225939_FluffyGC_rcu_vec
#define _headers_1691225939_FluffyGC_rcu_vec

#include <stddef.h>

#include "rcu/rcu_generic.h"
#include "rcu/rcu_generic_type.h"
#include "vec.h"

struct rcu_vec_head {
  size_t elementSize;
  // Offset to real vec_t
  size_t arrayOffset;
};

#define RCU_VEC_DEFINE_TYPE(name, t) \
  RCU_GENERIC_TYPE_DEFINE_TYPE(name, struct { \
    struct rcu_vec_head header; \
    vec_t(t) array; \
  })

// Gives expression which can be typeof
#define rcu_vec_element_type(list) \
  (*(list)->array.data)

extern struct rcu_generic_ops rcu_vec_t_ops;

#define rcu_vec_init(_self) do { \
  typeof(_self) self = (_self); \
  self->data->header.arrayOffset = offsetof(typeof(*self->data), array); \
  self->data->header.elementSize = sizeof(*self->data->array.data); \
  vec_init(&self->data->array); \
} while (0)

#define rcu_vec_init_rcu(rcu) \
  rcu_generic_type_init((rcu), &rcu_vec_t_ops)

#endif

