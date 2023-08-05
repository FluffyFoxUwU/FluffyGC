#ifndef _headers_1691206733_FluffyGC_rcu_vec
#define _headers_1691206733_FluffyGC_rcu_vec

// Generic container for RCU data
// doesn't really care what data its storing

#include "rcu/rcu.h"
#include "util/util.h"
#include <stdint.h>

typedef void (*rcu_generic_copy_func)(const void* src, void* dest);
typedef void (*rcu_generic_cleanup_func)(void* data);

struct rcu_generic_ops {
  // If NULL mean the data can be copied with memcpy
  rcu_generic_copy_func copy;
  
  // If NULL mean no-op
  rcu_generic_cleanup_func cleanup;
};

struct rcu_generic_container {
  struct rcu_head rcuHead;
  
  // Aligned pointer to the dataArray
  // as if by malloc (i.e. aligned to max_align_t)
  untyped_ptr data;
  
  // size of data contained by `data`
  size_t size;
  
  // Followed by user data
  uint8_t dataArray[];
};

struct rcu_generic {
  struct rcu_generic_ops* ops;
  struct rcu_struct rcu;
};

int rcu_generic_init(struct rcu_generic* self, struct rcu_generic_ops* ops);
void rcu_generic_cleanup(struct rcu_generic* self);

// Get readonly version
struct rcu_generic_container* rcu_generic_get_readonly(struct rcu_generic* self);

// Writing
void rcu_generic_write(struct rcu_generic* self, struct rcu_generic_container* new);

[[nodiscard("Leads to memory leak")]]
struct rcu_generic_container* rcu_generic_exchange(struct rcu_generic* self, struct rcu_generic_container* new);

// Get writeable copy
struct rcu_generic_container* rcu_generic_copy(struct rcu_generic* self);

// Container management
struct rcu_generic_container* rcu_generic_alloc_container(size_t size);
void rcu_generic_dealloc_container(struct rcu_generic_container* container);

/*
  struct data {
    int OwO;
  };
  
  struct rcu_generic_ops ops = {};
  struct rcu_generic data;
  rcu_generic_init(&data, &ops);
  
  {
    // Write
    
    // Get copy
    struct rcu_generic_container* copy = rcu_generic_copy(&data);
    struct data* casted = copy->data;
    casted->OwO = 8087;
    
    { 
      // Write #1
      // Overwrite, synchronize, clean and deallocate old structure
      rcu_generic_write(&data, copy);
    } 
    // or another way
    {
      // Write #2
      struct rcu_generic_container* old = rcu_generic_exchange(&data, copy);
      // Do cleanup on old->data
      rcu_generic_dealloc_container(old);
    }
  }
  
  {
    // Read #1
    
    rcu_read_lock(&data.rcu);
    
    // Get read version
    struct rcu_generic_container* container = rcu_generic_get_readonly(&data); 
    
    // Use data
    struct data* casted = container->data;
    (void) casted->OwO;
    
    rcu_read_unlock(&data.rcu, &container->rcuHead);
  }
  
  rcu_generic_cleanup(&data);
*/

#endif

