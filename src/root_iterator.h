#ifndef header_1656592486_7a874e10_7502_4318_a32d_7c8ce68d9445_root_iterator_h
#define header_1656592486_7a874e10_7502_4318_a32d_7c8ce68d9445_root_iterator_h

#include <stdbool.h>

// Root references iterator to keep me sane
//
// Iterates all known GC roots (thread
// roots, global roots, etc)
struct object_info;
struct heap;
struct region;
struct root_reference;

typedef void (^root_iterator_t)(struct object_info* object);
typedef void (^root_iterator2_t)(struct root_reference* ref, struct object_info* object);

void root_iterator_run(struct heap* heap, struct region* onlyIn, root_iterator_t iterator);
void root_iterator_run2(struct heap* heap, struct region* onlyIn, root_iterator2_t iterator);

#endif

