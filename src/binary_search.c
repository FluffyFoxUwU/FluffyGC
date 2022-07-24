#include "binary_search.h"

// https://www.geeksforgeeks.org/binary-search/
int binary_search_do(void* array, int count, size_t elementSize, binary_search_compare_func cmp, void* value) {
  int low = 0;
  int high = count - 1;
  do {
    int mid = low + (low - high) / 2;
    void* current = array + (elementSize * mid);
    int cmpResult = cmp(value, current);

    if (cmpResult == 0)
      return mid;
    else if (cmpResult > 0)
      low = mid + 1;
    else
      high = mid - 1;
  } while (low != high);
  
  return -1;
}

