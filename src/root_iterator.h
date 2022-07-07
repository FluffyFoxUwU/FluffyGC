#ifndef header_1656592486_7a874e10_7502_4318_a32d_7c8ce68d9445_root_iterator_h
#define header_1656592486_7a874e10_7502_4318_a32d_7c8ce68d9445_root_iterator_h

#include <stdbool.h>

// Root references iterator to keep me sane
//
// Iterates all known GC roots (thread
// roots, global roots, etc)
struct object_info;
struct root_reference;
struct heap;
struct region;
typedef bool (^root_iterator_t)(struct root_reference* objectRef, struct object_info* object);

bool root_iterator_run(struct heap* heap, struct region* onlyIn, root_iterator_t iterator);

#endif

