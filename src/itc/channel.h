#ifndef _headers_1685763386_FluffyGC_channel
#define _headers_1685763386_FluffyGC_channel

#include <stdint.h>

// Single direction
// Multiple writer may has some trouble
// because torn writes maybe?

struct channel {
  int readFD;
  int writeFD;
};

union channel_message_arg {
  void* pData;
  uint64_t u64Data;
  int iData;
};

struct channel_message {
  uint64_t identifier;
  uintptr_t cookie;
  union channel_message_arg args[5];
};

#define CHANNEL_MESSAGE(...) ((struct channel_message) {__VA_ARGS__})

struct channel* channel_new();
void channel_free(struct channel* self);

void channel_send(struct channel* self, struct channel_message* msg);
void channel_recv(struct channel* self, struct channel_message* msg);

#endif

