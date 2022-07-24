#ifndef header_1658403182_d14f079b_b2ea_4748_8817_b22323725fcc_cardtable_iterator_h
#define header_1658403182_d14f079b_b2ea_4748_8817_b22323725fcc_cardtable_iterator_h

#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

struct object_info;
struct heap;
typedef void (^cardtable_iterator)(struct object_info* info, int index);

// Also clear cardtable entry
// if it doesnt detects any pointer 
// of interest
void cardtable_iterator_do(struct heap* heap, struct object_info* objects, atomic_bool* cardTable, size_t cardTableSize, cardtable_iterator iterator);

#endif

