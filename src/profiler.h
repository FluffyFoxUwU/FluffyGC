#ifndef header_1657284954_ce0ac67c_833f_4bca_aa10_6cb7d5145e49_profiler_h
#define header_1657284954_ce0ac67c_833f_4bca_aa10_6cb7d5145e49_profiler_h

// A profiler
// not multithread safe
// as i dont understand
// how would that work out
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "collection/hashmap.h"
#include "collection/list.h"

struct profiler {
  bool currentlyProfiling;
  struct profiler_section* root;
  list_t* stack;
};

struct profiler_section {
  struct profiler* owner;
  struct profiler_section* parent;

  const char* name;

  list_node_t* thisNode;
  list_t* insertionOrder;
  HASHMAP(char, struct profiler_section) subsections;
 
  // In miliseconds
  bool isRunning;

  int enterCount;
  double totalDuration;
  double begin;
};

struct profiler* profiler_new();
void profiler_start(struct profiler* self);
void profiler_stop(struct profiler* self);

// Reset profiler
void profiler_reset(struct profiler* self);

void profiler_begin(struct profiler* self, const char* sectionName);
void profiler_end(struct profiler* self);

// Dump into format similiar to Minecraft's
// profiler output (only differences is section
// name is quoted and properly escaped)
//
// Read https://docs.minecraftforge.net/en/1.14.x/gettingstarted/debugprofiler/#reading-a-profiling-result
// (i dont like forge but this the best i can
// find about profiler format)
/*
 * Example output:
 * Time span: 70 ms
 * [0] "pre-collect" - 1.00% / 1.00% (0.70 ms)
 * [0] "collect" - 80.00% / 80.00% (56.00 ms)
 * [1] |   "mark" - 53.00% / 42.40% (29.68 ms)
 * [1] |   "sweep" - 47.00% / 37.60% (26.32 ms)
 * [0] "post-collect" - 19.00% / 19.00% (13.30 ms)
 * [1] |   "do-finalizers" - 90.00% / 17.10% (11.97 ms)
 * [1] |   "clear-card-table" - 10.00% / 1.90% (1.33 ms)
 */
void profiler_dump(struct profiler* self, FILE* output);

void profiler_free(struct profiler* self);

/*
struct profiler* profiler = profiler_new();
// Start profiling
profiler_start();

profiler_begin(profiler, "root");

// Work

profiler_end(profiler);

// Stop profiling
profiler_stop(profiler);

profiler_dump(profiler, stdout);
profiler_free(profiler);
*/

#endif

