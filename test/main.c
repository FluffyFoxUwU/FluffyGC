#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <flup/thread/thread.h>

#include "memory/arena.h"
#include "object/descriptor.h"
#include "object/object.h"

int main() {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  puts("Hello World!\n");
  
  struct arena* arena = arena_new(128 * 1024 * 1024);
  if (!arena) {
    puts("Error creating arena");
    return EXIT_FAILURE;
  }
  
  
  arena_free(arena);
  flup_thread_free(flup_detach_thread());
  // mimalloc_play();
}

