#ifndef header_1656115493_root_h
#define header_1656115493_root_h

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>

struct reference;
struct region_reference;

struct root_reference {
  // Used when CONFIG_DEBUG_DONT_REUSE_ROOT_REFERENCE
  // enabled
  struct root_reference* refToSelf;

  bool isValid;
  int index;

  struct root* owner;
  
  // When object moved GC will
  // reiterate all root entry and
  // update this
  struct region_reference* volatile data;

  pthread_t creator;
};

// Root is not thread safe
struct root {
  int size;
  struct root_reference* entries;
};

struct root* root_new(int initialSize);
void root_free(struct root* self);

struct root_reference* root_add(struct root* self,  struct region_reference* ref);

void root_clear(struct root* self);
void root_remove(struct root* self, struct root_reference* ref);

#endif

