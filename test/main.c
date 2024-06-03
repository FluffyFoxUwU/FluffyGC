#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <flup/core/logger.h>
#include <flup/thread/thread.h>

#include "heap/generation.h"

int main() {
  if (!flup_attach_thread("Main-Thread")) {
    fputs("Failed to attach thread\n", stderr);
    return EXIT_FAILURE;
  }
  
  pr_info("Hello World!");
  
  // Create 128 MiB generation
  struct generation* gen = generation_new(128 * 1024 * 1024);
  if (!gen) {
    pr_error("Error creating generation");
    return EXIT_FAILURE;
  }
  
  
  
  generation_free(gen);
  flup_thread_free(flup_detach_thread());
  // mimalloc_play();
}

