#ifndef header_1656770545_7827f843_bd3f_4236_9834_00cadd2220c9_marker_h
#define header_1656770545_7827f843_bd3f_4236_9834_00cadd2220c9_marker_h

#include <stdbool.h>
#include "builder.h"

struct object_info;
struct region;
struct gc_state;
struct gc_marker_args;

typedef void (^gc_mark_executor)(struct gc_marker_args args);

struct gc_marker_args {
  bool ignoreWeak;
  bool ignoreSoft;

  struct gc_state* gcState; 
  struct region* onlyIn;
  struct object_info* objectInfo;
  
  gc_mark_executor executor;
};

struct gc_marker_builder_struct {
  struct gc_marker_args data;
  
  BUILDER_DECLARE(struct gc_marker_builder_struct*,
      struct gc_state*, gc_state);
  BUILDER_DECLARE(struct gc_marker_builder_struct*,
      struct region*, only_in);
  BUILDER_DECLARE(struct gc_marker_builder_struct*,
      struct object_info*, object_info);
  BUILDER_DECLARE(struct gc_marker_builder_struct*,
      bool, ignore_weak);
  BUILDER_DECLARE(struct gc_marker_builder_struct*,
      bool, ignore_soft);
  BUILDER_DECLARE(struct gc_marker_builder_struct*,
      gc_mark_executor, executor);

  struct gc_marker_builder_struct* (^copy_from)(struct gc_marker_args args);
  struct gc_marker_args (^build)();
};

struct gc_marker_builder_struct* gc_marker_builder();

void gc_marker_mark(struct gc_marker_args args);

#endif

