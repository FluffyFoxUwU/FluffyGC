#ifndef header_1656592486_7a874e10_7502_4318_a32d_7c8ce68d9445_root_iterator_h
#define header_1656592486_7a874e10_7502_4318_a32d_7c8ce68d9445_root_iterator_h

#include <stdbool.h>

#include "builder.h"

// Root references iterator to keep me sane
//
// Iterates all known GC roots (thread
// roots, global roots, etc)
struct object_info;
struct heap;
struct region;
struct root_reference;

typedef void (^root_iterator_consumer_t)(struct root_reference* rootRef, struct object_info* object);

struct root_iterator_args {
  struct heap* heap;
  struct region* onlyIn;
  bool ignoreWeak;
  root_iterator_consumer_t consumer;
};

struct root_iterator_builder_struct {
  struct root_iterator_args data;
  
  BUILDER_DECLARE(struct root_iterator_builder_struct*, 
      bool, ignore_weak);
  BUILDER_DECLARE(struct root_iterator_builder_struct*, 
      struct heap*, heap);
  BUILDER_DECLARE(struct root_iterator_builder_struct*, 
      root_iterator_consumer_t, consumer);
  BUILDER_DECLARE(struct root_iterator_builder_struct*, 
      struct region*, only_in);
  
  struct root_iterator_args (^build)();
};

struct root_iterator_builder_struct* root_iterator_builder();

void root_iterator_run(struct root_iterator_args args);
//void root_iterator_run2(struct heap* heap, struct region* onlyIn, root_iterator2_t iterator);

#endif

