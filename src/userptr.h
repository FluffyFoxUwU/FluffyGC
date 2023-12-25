#ifndef header_1703422097_c03431ed_9fba_4b35_bd93_2e207ac80adc_userptr_h
#define header_1703422097_c03431ed_9fba_4b35_bd93_2e207ac80adc_userptr_h

#include "address_spaces.h"

// Use this for pointers which points to user data
struct userptr {
  void address_heap* ptr;
};

#define USERPTR(x) ((struct userptr) {(void address_heap*) (x)})
#define USERPTR_NULL USERPTR(NULL)

#endif

