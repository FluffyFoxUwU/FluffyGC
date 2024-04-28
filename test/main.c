#include <stdio.h>
#include <stdlib.h>

#include <flup/thread/thread.h>
#include <unistd.h>

#include "memory/arena.h"

int main() {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  puts("Hello World!\n");
  
  struct arena* arena = arena_new(128 * 1024 * 1024);
  if (!arena) {
    puts("Error creating arena\n");
    return EXIT_FAILURE;
  }
  
  arena_alloc(arena, 124);
  arena_alloc(arena, 123);
  arena_alloc(arena, 121);
  arena_wipe(arena);
  
  arena_free(arena);
  flup_thread_free(flup_detach_thread());
  // mimalloc_play();
}

