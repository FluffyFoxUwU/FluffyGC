#include <stdlib.h>
#include <assert.h>

#include "region.h"
#include "thread.h"
#include "root.h"
#include "config.h"

struct thread* thread_new(struct heap* heap, int id, int frameStackSize) {
  struct thread* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->topFramePointer = 0;
  self->frameStackSize = frameStackSize;
  self->heap = heap;
  self->id = id;
  self->topFrame = NULL;
  self->frames = NULL;

  self->frames = calloc(frameStackSize, sizeof(*self->frames));
  if (!self->frames)
    goto failure;

  thread_push_frame(self, CONFIG_DEFAULT_THREAD_STACK_SIZE);
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
  if (self->topFramePointer + 1 >= self->frameStackSize)
    goto stack_overflow;

  struct thread_frame* frame = &self->frames[self->topFramePointer];
  assert(!frame->isValid);

  if (frame->root == NULL) {
    frame->root = root_new(frameSize);
    if (frame->root == NULL)
      goto root_alloc_failed;
  }

  frame->isValid = true;
  self->topFrame = frame;
  self->topFramePointer++;
  return true;

  stack_overflow:
  root_alloc_failed:
  return false;
}

struct root_reference* thread_pop_frame(struct thread* self, struct root_reference* result) {
  // Cannot pop last frame
  // its needed to keep stuff simple by
  // guarante-ing that there atleast one
  // frame left
  if (self->topFramePointer <= 1)
    abort();
  
  // Reference come from somewhere else that is
  // not current frame
  if (result)
    assert(result->owner == self->topFrame->root);

  self->topFramePointer--;
  struct thread_frame* frame = &self->frames[self->topFramePointer];
  frame->isValid = false;

  struct root_reference* newRef = NULL;
  self->topFrame = &self->frames[self->topFramePointer - 1];
  if (result)
    newRef = root_add(self->topFrame->root, (struct region_reference*) result->data);
  
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

