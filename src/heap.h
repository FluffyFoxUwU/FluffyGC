#ifndef header_1656056358_heap_h
#define header_1656056358_heap_h

#include <pthread.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdarg.h>

#include "compiler_config.h"
#include "gc/gc_enums.h"

struct thread;
struct root;
struct descriptor;
struct region;
struct region_reference;
struct heap;
struct root_reference;
struct descriptor_typeid;
struct descriptor_field;

#define OBJECT_TYPES \
  X(OBJECT_TYPE_UNKNOWN, "unknown") \
  X(OBJECT_TYPE_NORMAL, "object") \
  X(OBJECT_TYPE_ARRAY, "reference array")

enum object_type {
# define X(t, ...) t,
  OBJECT_TYPES
# undef X
};

#define REPORT_TYPES \
  X(REPORT_YOUNG_ALLOCATION_FAILURE, "Young allocation failure") \
  X(REPORT_OLD_ALLOCATION_FAILURE, "Old allocation failure") \
  X(REPORT_PROMOTION_FAILURE, "Promotion failure") \
  X(REPORT_OUT_OF_MEMORY, "Out of memory") \
  X(REPORT_FULL_COLLECTION, "Explicit full collection")

enum report_type {
# define X(name, ...) name,
  REPORT_TYPES
# undef X
};

// How strong is the reference
enum reference_strength {
  REFERENCE_STRENGTH_STRONG,
  REFERENCE_STRENGTH_WEAK,
  REFERENCE_STRENGTH_SOFT,
};

typedef void (^heap_finalizer)(struct root_reference* reference);

struct object_info {
  struct heap* owner;
  bool isValid;

  // Atomic for future parallel marking and sweeping
  atomic_bool isMarked;

  enum object_type type;
  struct region_reference* regionRef;

  heap_finalizer finalizer;

  // Mark for old GC to not sweep
  // this for one old cycle
  // 
  // For protecting object which
  // recently promoted
  bool justMoved;

  // Counted during marking phase 
  // and reseted after GC cycle
  //
  // Used to determine whether
  // weak or soft reference should
  // be cleared
  atomic_int strongRefCount;

  union {
    struct {
      // Like one sized array can be abused
      // to make weak or soft reference to
      // anything else UwU
      enum reference_strength strength;
      int size; 
    } array;

    struct {
       struct descriptor* desc;
    } normal;
  } typeSpecific;

  bool isMoved;
  struct {
    struct region_reference* oldLocation;
    struct object_info* oldLocationInfo;
    struct region_reference* newLocation;
    struct object_info* newLocationInfo;
  } moveData;
};

struct thread_data {
  struct thread* thread;
  int numberOfTimeEnteredUnsafeGC;
};

struct heap {
  struct gc_state* gcState;
  
  // Increases concurrency of allocations
  // by not forcing serialization on every
  // allocation unless needed
  pthread_mutex_t memoryExhaustionLock;

  struct region* youngGeneration;
  struct region* oldGeneration;

  struct object_info* youngObjects;
  struct object_info* oldObjects;
  
  size_t oldToYoungCardTableSize;
  size_t youngToOldCardTableSize;
  
  // Atomic because there is
  // multiple writes (reminder
  // for myself to not keep
  // converting bool to atomic_bool 
  // and to bool repeatly)
  atomic_bool* oldToYoungCardTable;
  atomic_bool* youngToOldCardTable;

  int localFrameStackSize;
  size_t preTenurSize;

  // Percentage of heap before telling GC thread
  // to start old concurrent  GCcycle (0.45 
  // mean start when usage reach 45%
  // of old generation)
  float concurrentOldGCthreshold;
  
  ///////////////////////////////////////////////
  // For sending signal that request completed //
  ///////////////////////////////////////////////
  volatile bool gcCompleted; 
  
  pthread_mutex_t gcCompletedLock;
  pthread_cond_t gcCompletedCond;
  ///////////////////////////////////////////////

  //////////////////////////////////////////
  // For sending request to the GC thread //
  //////////////////////////////////////////
  volatile bool gcRequested; 
  volatile enum gc_request_type gcRequestedType;
  
  pthread_mutex_t gcMayRunLock;
  pthread_cond_t gcMayRunCond;
  //////////////////////////////////////////
 
  pthread_mutex_t callGCLock;

  // If there any writer
  // GC is not safe to run
  // 
  // honestly i never success making
  // this without abusing rwlock
  pthread_rwlock_t gcUnsafeRwlock;

  // Thread informations
  pthread_key_t currentThreadKey;
  
  // Metaspace size
  size_t metaspaceSize;
  
  pthread_rwlock_t globalRootRWLock;
  struct root* globalRoot;

  // All fields below this rwlock are protected
  // by it 
  pthread_rwlock_t lock;

  // These may be update by other threads
  // for why volatile
  size_t metaspaceUsage;

  int threadsCount;
  int threadsListSize;
  struct thread** threads;
  
  bool memoryExhaustionLockInited;
  bool gcCompletedLockInited;
  bool gcCompletedCondInited;
  bool gcMayRunLockInited;
  bool gcMayRunCondInited;
  bool callGCLockInited;
  bool gcUnsafeRwlockInited;
  bool currentThreadKeyInited;
  bool globalRootRWLockInited;
  bool lockInited;
};

struct heap* heap_new(size_t youngSize, size_t oldSize,
                      size_t metaspaceSize,
                      int localFrameStackSize,
                      float concurrentOldGCthreshold,
                      int globalRootSize);
void heap_free(struct heap* self);

struct descriptor* heap_descriptor_new(struct heap* self, struct descriptor_typeid id, size_t objectSize, int numFields, struct descriptor_field* fields);
void heap_descriptor_release(struct heap* self, struct descriptor* desc);

// For convenience writing
// pointers
void heap_obj_write_ptr(struct heap* self, struct root_reference* object, size_t offset, struct root_reference* child);
struct root_reference* heap_obj_read_ptr(struct heap* self, struct root_reference* object, size_t offset);

// undefined behaviour when 
// write/read is overlaps 
// with a pointer field
void heap_obj_write_data(struct heap* self, struct root_reference* object, size_t offset, void* data, size_t size);
void heap_obj_read_data(struct heap* self, struct root_reference* object, size_t offset, void* data, size_t size);

void heap_array_write(struct heap* self, struct root_reference* object, int index, struct root_reference* child);
struct root_reference* heap_array_read(struct heap* self, struct root_reference* object, int index);

// These two controls whether GC can run
// or not 
void heap_enter_unsafe_gc(struct heap* self);
void heap_exit_unsafe_gc(struct heap* self);

// Block if GC currently running
void heap_wait_gc(struct heap* self);

void heap_call_gc(struct heap* self, enum gc_request_type requestType);
void heap_call_gc_blocking(struct heap* self, enum gc_request_type requestType);

// Threads stuffs
bool heap_is_attached(struct heap* self);
bool heap_attach_thread(struct heap* self);
void heap_detach_thread(struct heap* self);
bool heap_resize_threads_list(struct heap* self, int newSize);
bool heap_resize_threads_list_no_lock(struct heap* self, int newSize);
struct thread_data* heap_get_thread_data(struct heap* self);

// Object allocations
struct root_reference* heap_obj_new(struct heap* self, struct descriptor* desc);
struct root_reference* heap_array_new(struct heap* self, int size);

// Arbitrary size object with no pointer
// in it (still need write data and read
// data functions)
struct root_reference* heap_obj_opaque_new(struct heap* self, size_t size);

// Reset all fields
void heap_reset_object_info(struct heap* self, struct object_info* info);

// Gets
struct region_reference* heap_get_region_ref(struct heap* self, void* data);
struct region* heap_get_region(struct heap* self, void* data);
struct region* heap_get_region2(struct heap* self, struct root_reference* data);
struct object_info* heap_get_object_info(struct heap* heap, struct region_reference* ref);

// Events
void heap_sweep_an_object(struct heap* self, struct object_info* obj);

// Misc
bool heap_is_array(struct heap* self, struct root_reference* ref);
bool heap_can_record_in_cardtable(struct heap* self, struct object_info* obj, size_t offset, struct object_info* data);

// Reports
ATTRIBUTE((format(printf, 2, 3)))
void heap_report_printf(struct heap* self, const char* fmt, ...); 
void heap_report_vprintf(struct heap* self, const char* fmt, va_list list); 
void heap_report_gc_cause(struct heap* self, enum report_type type);

// Misc
const char* heap_tostring_object_type(struct heap* self, enum object_type type);

#endif


