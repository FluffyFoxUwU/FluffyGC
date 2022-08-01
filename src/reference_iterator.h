#ifndef header_1659354094_766f7641_4b7c_4ec1_9159_1567e168d506_reference_iterator_h
#define header_1659354094_766f7641_4b7c_4ec1_9159_1567e168d506_reference_iterator_h

// Iterate references in objects
#include <stddef.h>
#include <stdbool.h>

#include "builder.h"

struct object_info;
struct heap;

// `data` maybe be null if unavailable
typedef void (^reference_consumer_t)(struct object_info* data, void* ptr, size_t offset);

struct reference_iterator_args {
  bool ignoreWeak;
  bool ignoreStrong;
  bool ignoreSoft;
  struct object_info* object;
  struct heap* heap;
  reference_consumer_t consumer;
};

struct reference_iterator_builder {
  struct reference_iterator_args data;

  BUILDER_DECLARE(struct reference_iterator_builder*,
      bool, ignore_soft);
  BUILDER_DECLARE(struct reference_iterator_builder*,
      bool, ignore_weak);
  BUILDER_DECLARE(struct reference_iterator_builder*,
      bool, ignore_strong);
  BUILDER_DECLARE(struct reference_iterator_builder*,
      struct heap*, heap);
  BUILDER_DECLARE(struct reference_iterator_builder*,
      struct object_info*, object);
  BUILDER_DECLARE(struct reference_iterator_builder*,
      reference_consumer_t, consumer);

  struct reference_iterator_builder* (^copy_from)(struct reference_iterator_args args);
  struct reference_iterator_args (^build)();
};

struct reference_iterator_builder* reference_iterator_builder();
void reference_iterator_run(struct reference_iterator_args args);

#endif

