#include <stddef.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include <FluffyGC/version.h>
#include <FluffyGC/descriptor.h>
#include <FluffyGC/object.h>
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
  
  fluffygc_state* heap = fluffygc_new(50 * 1024 * 1024, &listOfGCs[0]);
  if (!heap) {
    printf("[ERROR] Cannot create the heap!");
    abort();
  }
  free(listOfGCs);
  
  struct message {
    int integer;
    char data[];
  };
  
  static const fluffygc_descriptor descriptor = {
    .fieldCount = 0,
    .hasFlexArrayField = false,
    .objectSize = sizeof(struct message)
  };
  
  static const fluffygc_descriptor arrayOfMessagesDescriptor = {
    .fieldCount = 0,
    .hasFlexArrayField = true,
    .objectSize = 0
  };
  
  const int PRESERVED_MESSAGE_COUNT = 5;
  fluffygc_object* arrayOfMessages = fluffygc_object_new(heap, &arrayOfMessagesDescriptor, sizeof(void*) * PRESERVED_MESSAGE_COUNT);
  
  for (int i = 0; i < PRESERVED_MESSAGE_COUNT; i++) {
    size_t messageSize = snprintf(NULL, 0, "Preserved message ID %d UwU", i) + 1;
    fluffygc_object* msgRef = fluffygc_object_new(heap, &descriptor, messageSize);
    
    struct message* message = fluffygc_object_get_data_ptr(msgRef);
    snprintf(message->data, messageSize, "Preserved message ID %d UwU", i);
    message->integer = i;
    
    fluffygc_object_write_ref(heap, arrayOfMessages, i * sizeof(void*), msgRef);
    fluffygc_object_unref(heap, msgRef);
  }
  
  // Trigger the GC
  for (int i = 0; i < (40 * 1024 * 1024) / descriptor.objectSize; i++) {
    fluffygc_object* msgRef = fluffygc_object_new(heap, &descriptor, 0);
    struct message* msg = fluffygc_object_get_data_ptr(msgRef);
    msg->integer = 2983878;
    fluffygc_object_unref(heap, msgRef);
  }
  
  // Access the preserved message to see GC hasnt nuke them
  for (int i = 0; i < PRESERVED_MESSAGE_COUNT; i++) {
    fluffygc_object* msgRef = fluffygc_object_read_ref(heap, arrayOfMessages, i * sizeof(void*));
    struct message* msg = fluffygc_object_get_data_ptr(msgRef);
    printf("Preserved message[%d]->integer = %d\n", i, msg->integer);
    printf("Preserved message[%d]->data = %s\n", i, msg->data);
    fluffygc_object_unref(heap, msgRef);
  }
  
  fluffygc_object_unref(heap, arrayOfMessages);
  fluffygc_free(heap);
  return 0;
  return fluffygc_impl_main();
}


