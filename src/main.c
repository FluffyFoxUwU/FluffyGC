#include "bug.h"
#include "soc.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
layout of object
struct object {
  struct object* forwardingPointer;
  struct descriptor* descriptor;
  size_t objectSize; // Represent size of data in payload (including redzones)
  int age; // Number of collection survived
  
  char payload[];
}

actual writable address is `obj->payload + obj->redzoneSize` to 
account redzones which are implemented using ASAN poisoning facility
*/

struct obj {
  long uwu;
  void* owo;
  int fur;
  double furry;
};

int main2(int argc, char** argv) {
  printf("Hello World!\n");
  
  struct thread* thread = thread_new();
  
  thread_free(thread);
  return EXIT_SUCCESS;
}

