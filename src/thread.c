#include <stdlib.h>
#include <assert.h>

#include "reference.h"
#include "region.h"
#include "thread.h"
#include "root.h"

struct thread* thread_new(struct heap* heap, int id, int frameStackSize) {
  struct thread* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->framePointer = 0;
  self->frameStackSize = frameStackSize;
  self->heap = heap;
  self->id = id;

  self->frames = calloc(frameStackSize, sizeof(*self->frames));
  if (!self->frames)
    goto failure;

  return self;
  
  failure:
  thread_free(self);
  return NULL;
}

void thread_free(struct thread* self) {
  if (!self)
    return;

  for (int i = 0; i < self->frameStackSize; i++)
    root_free(self->frames[i].root);
  
  free(self->frames);
  free(self);
}

bool thread_push_frame(struct thread* self, int frameSize) {
  struct thread_frame* frame = &self->frames[self->framePointer];
  assert(!frame->isValid);

  if (frame->root == NULL) {
    frame->root = root_new(frameSize);
    if (frame->root == NULL)
      return false;
  }

  frame->isValid = true;
  self->topFrame = frame;
  self->framePointer++;
  return true;
}

struct root_reference* thread_pop_frame(struct thread* self, struct root_reference* result) {
  if (self->framePointer <= 0)
    abort();

  self->framePointer--;
  struct thread_frame* frame = &self->frames[self->framePointer];
  frame->isValid = false;

  struct root_reference* newRef = NULL;
  if (self->framePointer > 0) {
    self->topFrame = &self->frames[self->framePointer - 1];
    newRef = root_add(self->topFrame->root, (struct region_reference*) result->data);
  } else {
    self->topFrame = NULL;
  }
  
  root_clear(frame->root);  
  return newRef;
}

struct root_reference* thread_local_add2(struct thread* self, struct root_reference* ref) {
  return root_add(self->topFrame->root, (struct region_reference*) ref->data);
}

struct root_reference* thread_local_add(struct thread* self, struct region_reference* ref) {
  return root_add(self->topFrame->root, ref);
}

void thread_local_remove(struct thread* self, struct root_reference* ref) {
  root_remove(self->topFrame->root, ref);
}

