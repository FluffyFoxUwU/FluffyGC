#ifndef header_1656163794_thread_h
#define header_1656163794_thread_h

#include <stdbool.h>
#include <stddef.h>

struct descriptor;
struct root;
struct heap;
struct root_reference;
struct region_reference;

struct thread {
  int id;
  struct heap* heap;

  int frameStackSize;
  int framePointer;
  struct thread_frame* topFrame;
  struct thread_frame* frames;
};

struct thread_frame {
  bool isValid;
  struct root* root;
};

struct thread* thread_new(struct heap* heap, int id, int frameStackSize);
void thread_free(struct thread* self);

// Add ref to local frame and return
// local reference
struct root_reference* thread_local_add(struct thread* self, struct region_reference* ref);
struct root_reference* thread_local_add2(struct thread* self, struct root_reference* ref);
void thread_local_remove(struct thread* self, struct root_reference* ref);

bool thread_push_frame(struct thread* self, int frameSize);
struct root_reference* thread_pop_frame(struct thread* self, struct root_reference* result);

#endif

