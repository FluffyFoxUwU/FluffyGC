#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "profiler.h"

static void freeSection(struct profiler_section* section);
static struct profiler_section* newSection(struct profiler* self, const char* name) {
  struct profiler_section* section = malloc(sizeof(*section));
  if (!section)
    goto failure;

  section->enterCount = 0;
  section->begin = 0;
  section->isRunning = false;
  section->totalDuration = 0.0f;
  section->owner = self;
  section->thisNode = NULL;
  section->name = NULL;
  section->insertionOrder = NULL;

  section->name = strdup(name);
  if (!section->name)
    goto failure;

  hashmap_init(&section->subsections, hashmap_hash_string, strcmp);
  section->insertionOrder = list_new();
  if (!section->insertionOrder)
    goto failure;

  return section;

  failure:
  freeSection(section);
  return NULL;
}

static void freeSection(struct profiler_section* section) {
  if (!section)
    return;
  if (section->isRunning)
    abort();

  if (section->insertionOrder) {
    list_iterator_t* it = list_iterator_new(section->insertionOrder, LIST_HEAD);
    list_node_t* node = NULL;
    if (!it)
      abort();

    while ((node = list_iterator_next(it)))
      freeSection(node->val);
    list_iterator_destroy(it);

    list_destroy(section->insertionOrder);
  }
  hashmap_cleanup(&section->subsections);
  
  free((char*) section->name);
  free(section);
}

static double getCurrentTime() {
  struct timespec current = {
    .tv_nsec = 0,
    .tv_sec = 0
  };
  clock_gettime(CLOCK_REALTIME, &current);

  double time = ((double) current.tv_sec) + 
              (((double) current.tv_nsec) / 1000000000.0f);
  return time * 1000.0f;
}

static void startSection(struct profiler_section* section) {
  if (section->isRunning)
    abort();

  section->begin = getCurrentTime();
  section->enterCount++;
  section->isRunning = true;
}

static void endSection(struct profiler_section* section) {
  if (!section->isRunning)
    abort();

  section->isRunning = false;
  double duration = getCurrentTime() - section->begin;
  section->totalDuration += duration;
}

static void dumpSection(struct profiler_section* section, int indent, FILE* output) {
  if (section->isRunning)
    abort();

  if (section == section->owner->root) {
    indent--;
  } else {
    fprintf(output, "[%d] ", indent);
    for (int i = 0; i < indent; i++)
      fprintf(output, "|  ");
    
    double timeInParent = (section->totalDuration / section->parent->totalDuration) * 100;
    double timeInTotal =  (section->totalDuration / section->owner->root->totalDuration) * 100;
    fprintf(output, "\"%s\" - %.2lf%% / %.2f%%   (%.2f ms)\n", section->name, timeInParent, timeInTotal, section->totalDuration);
  }
  
  list_iterator_t* it = list_iterator_new(section->insertionOrder, LIST_HEAD);
  list_node_t* node = NULL;
  if (!it)
    abort();

  while ((node = list_iterator_next(it)))
    dumpSection(node->val, indent + 1, output);
  list_iterator_destroy(it);
}

struct profiler* profiler_new() {
  struct profiler* self = malloc(sizeof(*self));
  if (!self)
    goto failure;
  
  self->currentlyProfiling = false;
  self->root = NULL;
  self->stack = NULL;

  self->stack = list_new();
  if (!self->stack)
    goto failure;

  self->root = newSection(self, "root");
  if (!self->root)
    goto failure;
  self->root->parent = self->root;
  return self;

  failure:
  profiler_free(self);
  return NULL;
}

void profiler_start(struct profiler* self) {
  startSection(self->root);
  list_node_t* node = list_node_new(self->root);
  if (!node)
    abort();
  list_rpush(self->stack, node);
  self->currentlyProfiling = true;
}

void profiler_stop(struct profiler* self) {
  if (!self->currentlyProfiling)
    abort();
  
  self->currentlyProfiling = false;
  if (self->stack->len > 1)
    abort();
  endSection(self->root);
  list_remove(self->stack, self->stack->tail);
}

void profiler_dump(struct profiler* self, FILE* output) {
  if (self->currentlyProfiling)
    abort();
  
  if (self->root->isRunning)
    abort();

  fprintf(output, "Time span: %.2lf ms\n", self->root->totalDuration);
  dumpSection(self->root, 0, output);
}

void profiler_begin(struct profiler* self, const char* sectionName) {
  if (!self->currentlyProfiling)
    abort();
  
  if (!self->currentlyProfiling)
    abort();

  struct profiler_section* current = self->stack->tail->val; 
  struct profiler_section* existing = hashmap_get(&current->subsections, sectionName);
  if (!existing) {
    existing = newSection(self, sectionName);
    existing->parent = current;
    hashmap_put(&current->subsections, existing->name, existing);
    
    list_node_t* node = list_node_new(existing);
    if (!node)
      abort();
    list_rpush(current->insertionOrder, node);
  }
  
  startSection(existing);

  list_node_t* node = list_node_new(existing);
  if (!node)
    abort();
  list_rpush(self->stack, node);
}

void profiler_end(struct profiler* self) {
  if (!self->currentlyProfiling)
    abort();
  
  struct profiler_section* current = self->stack->tail->val;
  if (current == self->root)
    abort();

  list_remove(self->stack, self->stack->tail);
  endSection(current);
}

void profiler_free(struct profiler* self) {
  if (self->currentlyProfiling)
    abort();
  
  if (!self)
    return;

  // There always one left which is
  // the root
  if (self->stack->len > 1)
    abort();
  
  freeSection(self->root);
  list_destroy(self->stack);
  free(self);
}

void profiler_reset(struct profiler* self) {
  if (self->currentlyProfiling)
    abort();
  freeSection(self->root);
  
  self->root = newSection(self, "root");
  if (!self->root)
    abort();
  self->root->parent = self->root;
}

