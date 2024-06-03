#ifndef UWU_EC05524B_8358_41C8_A9FA_C6CC3902DB3F_UWU
#define UWU_EC05524B_8358_41C8_A9FA_C6CC3902DB3F_UWU

struct bitmap {
  unsigned long bitCount;
  _Atomic(unsigned long)* map;
};

struct bitmap* bitmap_new(unsigned long bitCount);
void bitmap_free(struct bitmap* self);

// Set a bit with new bit (strong mode)
// return previous bit value
bool bitmap_set(struct bitmap* self, unsigned long bitIndex, bool new);
bool bitmap_test(struct bitmap* self, unsigned long bitIndex);

#endif
