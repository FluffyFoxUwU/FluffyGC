#ifndef header_1656116009_reference_h
#define header_1656116009_reference_h

struct region_reference;
struct reference;

typedef enum {
  REFERENCE_INVALID,
  REFERENCE_STRONG,
  REFERENCE_ROOT
} reference_type_t;

struct reference {
  reference_type_t type;
  union {
    struct region_reference* strong;
    struct root_reference* root;
  } data;
};

struct reference* reference_new_strong(struct region_reference* ref);
struct reference* reference_new_root(struct root_reference* ref);

struct region_reference* reference_get(struct reference* self);
struct root_reference* reference_get_root(struct reference* self);

void reference_free(struct reference* self);

#endif

