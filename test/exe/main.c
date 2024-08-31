#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include <FluffyGC/FluffyGC.h>
#include <FluffyGC/gc.h>

[[gnu::visibility("default")]]
extern int fluffygc_impl_main();

int main() {
  printf("Compiled with header version 0x%X\n", FLUFFYGC_API_VERSION);
  printf("Linked with FluffyGC version 0x%X implementing API version 0x%X\n", fluffygc_get_impl_version(), fluffygc_get_api_version());
  printf("Description of implementation is '%s'\n", fluffygc_get_impl_version_description());
  
  size_t numberOfGCs = fluffygc_gc_get_algorithmns(NULL, 0);
  if (numberOfGCs == 0) {
    printf("[ERROR] No GC is implemented\n");
    abort();
  }
  
  fluffygc_gc* listOfGCs = calloc(numberOfGCs, sizeof(*listOfGCs));
  if (!listOfGCs) {
    printf("[ERROR] Cannot allocate memory for storing list of GCs\n");
    abort();
  }
  fluffygc_gc_get_algorithmns(listOfGCs, numberOfGCs);
  
  for (size_t i = 0; i < numberOfGCs; i++) {
    fluffygc_gc* current = &listOfGCs[i];
    printf("Index %zu:\n"
           "\tID: %s\n"
           "\tName: %s\n"
           "\tVersion: 0x%x\n"
           "\tDescription: %s\n",
           i, current->id, current->name, current->version, current->description);
  }
  
  fluffygc_state* heap = fluffygc_new(768 * 1024 * 1024, &listOfGCs[0]);
  if (!heap) {
    printf("[ERROR] Cannot create the heap!");
    abort();
  }
  free(listOfGCs);
  
  fluffygc_free(heap);
  return 0;
  return fluffygc_impl_main();
}


