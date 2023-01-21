#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "profiler.h"
#include "bug.h"

static void freeSection(struct profiler_section* section) {
  if (!section)
    return;
  if (section->isRunning)
    BUG();

  if (section->insertionOrder) {
    list_node_t* node = section->insertionOrder->head;
    while (node && (node = node->next))
      freeSection(node->val);
    list_destroy(section->insertionOrder);
  }
  hashmap_cleanup(&section->subsections);
  
  free((char*) section->name);
  free(section);
}

static struct profiler_section* newSection(struct profiler* self, const char* name) {
  struct profiler_section* section = malloc(sizeof(*section));
  if (!section)
    goto failure;
  *section = (struct profiler_section) {};

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
    BUG();

  section->begin = getCurrentTime();
  section->enterCount++;
  section->isRunning = true;
}

static void endSection(struct profiler_section* section) {
  if (!section->isRunning)
    BUG();

  section->isRunning = false;
  double duration = getCurrentTime() - section->begin;
  section->totalDuration += duration;
}

static void dumpSection(struct profiler_section* section, int indent, FILE* output) {
  if (section->isRunning)
    BUG();

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
  
  list_node_t* node = section->insertionOrder->head;
  while (node && (node = node->next))
    dumpSection(node->val, indent + 1, output);
}

struct profiler* profiler_new() {
  struct profiler* self = malloc(sizeof(*self));
  if (!self)
    goto failure;
  *self = (struct profiler) {};
  
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
    BUG();
  list_rpush(self->stack, node);
  self->currentlyProfiling = true;
}

void profiler_stop(struct profiler* self) {
  if (!self->currentlyProfiling)
    BUG();
  
  self->currentlyProfiling = false;
  if (self->stack->len > 1)
    BUG();
  endSection(self->root);
  list_remove(self->stack, self->stack->tail);
}

void profiler_dump(struct profiler* self, FILE* output) {
  if (self->currentlyProfiling)
    BUG();
  
  if (self->root->isRunning)
    BUG();

  fprintf(output, "Time span: %.2lf ms\n", self->root->totalDuration);
  dumpSection(self->root, 0, output);
}

void profiler_begin(struct profiler* self, const char* sectionName) {
  if (!self->currentlyProfiling)
    BUG();
  
  if (!self->currentlyProfiling)
    BUG();

  struct profiler_section* current = self->stack->tail->val; 
  struct profiler_section* existing = hashmap_get(&current->subsections, sectionName);
  if (!existing) {
    existing = newSection(self, sectionName);
    existing->parent = current;
    hashmap_put(&current->subsections, existing->name, existing);
    
    list_node_t* node = list_node_new(existing);
    if (!node)
      BUG();
    list_rpush(current->insertionOrder, node);
  }
  
  startSection(existing);

  list_node_t* node = list_node_new(existing);
  if (!node)
    BUG();
  list_rpush(self->stack, node);
}

void profiler_end(struct profiler* self) {
  if (!self->currentlyProfiling)
    BUG();
  
  struct profiler_section* current = self->stack->tail->val;
  if (current == self->root)
    BUG();

  list_remove(self->stack, self->stack->tail);
  endSection(current);
}

void profiler_free(struct profiler* self) {
  if (self->currentlyProfiling)
    BUG();
  
  if (!self)
    return;

  // There always one left which is
  // the root
  if (self->stack->len > 1)
    BUG();
  
  freeSection(self->root);
  list_destroy(self->stack);
  free(self);
}

void profiler_reset(struct profiler* self) {
  if (self->currentlyProfiling)
    BUG();
  freeSection(self->root);
  
  self->root = newSection(self, "root");
  if (!self->root)
    BUG();
  self->root->parent = self->root;
}

