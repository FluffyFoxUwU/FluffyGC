#include "cardtable_iterator.h"

#include "../config.h"
#include "../heap.h"
#include <stdatomic.h>

void cardtable_iterator_do(struct heap* heap, struct object_info* objects, atomic_bool* cardTable, size_t cardTableSize, cardtable_iterator iterator) {
  for (int i = 0; i < cardTableSize; i++) { 
    if (atomic_load(&cardTable[i]) == false)
      continue;
    
    int rangeStart = i * CONFIG_CARD_TABLE_PER_BUCKET_SIZE;

    bool encounterAny = false;    
    for (int j = 0; j < CONFIG_CARD_TABLE_PER_BUCKET_SIZE; j++) {
      struct object_info* info = &objects[rangeStart + j];
       
      if (!info->regionRef || info->isValid == false)
        continue;

      iterator(info, i);
      encounterAny = true;
    }

    if (!encounterAny)
      atomic_store(&cardTable[i], false);
  }
}

