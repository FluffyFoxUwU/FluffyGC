#ifndef _headers_1691210445_FluffyGC_rcu_generic_type
#define _headers_1691210445_FluffyGC_rcu_generic_type

#include <stddef.h>

#include "rcu/rcu.h"
#include "rcu_generic.h"

// RCU generic but type checked

#define RCU_GENERIC_TYPE_DEFINE_TYPE(name, type) \
  struct name { \
    struct rcu_generic generic; \
    type* dataType; \
    struct name ## _readonly_container* readContainerType; \
    struct name ## _writeable_container* writeContainerType; \
  }; \
  struct name ## _readonly_container { \
    struct rcu_generic_container* container; \
    const type* data; \
  }; \
  struct name ## _writeable_container { \
    struct rcu_generic_container* container; \
    type* data; \
  };

#define rcu_generic_type_init(self, ops) rcu_generic_init(&(self)->generic, ops)
#define rcu_generic_type_cleanup(self) rcu_generic_cleanup(&(self)->generic)

#define rcu_generic_type_get(_self) \
  (*((^typeof(*(_self)->readContainerType)* (typeof(_self) self, typeof(*(_self)->readContainerType)* x) { \
    struct rcu_generic_container* container = rcu_generic_get_readonly(&(self)->generic); \
    if (!container) \
      return NULL; \
    x->container = container; \
    x->data = container->data.ptr; \
    return x; \
  })(_self, &(typeof(*(_self)->readContainerType)) {})))

#define rcu_generic_type_exchange(_self, new) \
  (*((^typeof(*(_self)->writeContainerType)* (typeof(_self) self, typeof(*(self)->writeContainerType)* x) { \
    typeof(*(self)->writeContainerType)* newVar = (new); \
    struct rcu_generic_container* container = rcu_generic_exchange(&(self)->generic, newVar ? newVar->container : NULL); \
    if (!container) \
      return NULL; \
    x->container = container; \
    x->data = container->data.ptr; \
    return x; \
  })(_self, &(typeof(*(_self)->writeContainerType)) {})))

#define rcu_generic_type_write(_self, new) do {\
  typeof((_self)->writeContainerType) a = (new); \
  rcu_generic_write(&(_self)->generic, a ? a->container : NULL); \
} while (0)

// This is fine, https://godbolt.org/z/dfzrxvP1r
// Clang can inline Block calls
#define rcu_generic_type_alloc_for(self) \
  (*((^typeof(*(self)->writeContainerType)* (typeof(*(self)->writeContainerType)* x) { \
    struct rcu_generic_container* container = rcu_generic_alloc_container(sizeof(*x->data)); \
    if (!container) \
      return NULL; \
    x->container = container; \
    x->data = container != NULL ? container->data.ptr : NULL; \
    return x; \
  })(&(typeof(*(self)->writeContainerType)) {})))

#define rcu_generic_type_get_rcu(self) \
  (&(self)->generic.rcu)

#define rcu_generic_type_copy(self) \
  (*((^typeof(*(self)->writeContainerType)* (typeof(*(self)->writeContainerType)* x) { \
    struct rcu_generic_container* container = rcu_generic_copy(&(self)->generic); \
    if (!container) \
      return NULL; \
    x->container = container; \
    x->data = container->data.ptr; \
    return x; \
  })(&(typeof(*(self)->writeContainerType)) {})))

/*
struct some_data {
  int a;
  bool uwu;
};

RCU_GENERIC_TYPE_DEFINE_TYPE(some_data_rcu, struct some_data);
struct rcu_generic_ops ops = {};

void a() {
  struct some_data_rcu data;
  rcu_generic_type_init(&data, &ops);
  
  rcu_read_lock(&data.generic.rcu);
  struct some_data_rcu_readonly_container readResult = rcu_generic_type_get(&data);
  rcu_read_unlock(&data.generic.rcu);
  
  struct some_data_rcu_writeable_container writeable = rcu_generic_type_alloc_for(&data);
  writeable.data->uwu = 8087;
  rcu_generic_write(&data.generic, writeable.container);
  
  struct some_data_rcu_writeable_container new = rcu_generic_type_alloc_for(&data);
  new.data->uwu = 928;
  rcu_generic_dealloc_container(new.container);
  
  rcu_generic_type_cleanup(&data);
}
*/

#endif

