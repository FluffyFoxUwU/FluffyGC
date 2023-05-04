#ifndef _headers_1683201940_FluffyGC_thread_local
#define _headers_1683201940_FluffyGC_thread_local

#include <pthread.h>

// Can't use thread_local *sad fox noises*
struct thread_local_struct {
  pthread_key_t key;
  bool inited;
};

int thread_local_init(struct thread_local_struct* self, void (*destructor)(void* udata));
void thread_local_cleanup(struct thread_local_struct* self);

void* thread_local_get(struct thread_local_struct* self);
void thread_local_set(struct thread_local_struct* self, void* udata);

#endif

