#include <stdio.h>

#include "api/headers/FluffyGC.h"

[[gnu::visibility("default")]]
extern int fluffygc_impl_main();

int main() {
  printf("Compiled with header version 0x%X\n", FLUFFYGC_API_VERSION);
  printf("Linked with FluffyGC version 0x%X implementing API version 0x%X\n", fluffygc_get_impl_version(), fluffygc_get_api_version());
  printf("Description of implementation is '%s'\n", fluffygc_get_impl_version_description());
  
  return 0;
  return fluffygc_impl_main();
}


