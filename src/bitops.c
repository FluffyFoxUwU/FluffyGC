#include "bitops.h"

void setbit(long nr, volatile unsigned long* addr) {
  addr[BIT_WORD(nr)] |= BIT_MASK(nr);
}

void clearbit(long nr, volatile unsigned long* addr) {
  addr[BIT_WORD(nr)] &= ~BIT_MASK(nr);
}

bool testbit(long nr, volatile unsigned long* addr) {
  return (addr[BIT_WORD(nr)] & BIT_MASK(nr)) != 0;
}

int findbit(bool state, volatile unsigned long* addr, unsigned int count) {
  unsigned long skipWord = state ? 0 : ~1UL;
  for (unsigned int bitCurrent = 0; bitCurrent < count; bitCurrent += BITS_PER_LONG) {
    // Skipping BITS_PER_LONG at once
    if (addr[BIT_WORD(bitCurrent)] == skipWord)
      continue;
    
    // Testing each bit
    for (; bitCurrent < count; bitCurrent++)
      if (testbit(bitCurrent, addr) == state)
        return bitCurrent;
  }
  return -1;
}
