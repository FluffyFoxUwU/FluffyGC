#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <inttypes.h>

#include "helper.h"
#include "FluffyHeap.h"
#include "api/mods/debug/common.h"
#include "api/mods/debug/debug.h"
#include "api/mods/dma_common.h"
#include "buffer.h"
#include "concurrency/thread_local.h"
#include "logger/logger.h"
#include "api/api.h"
#include "mods/dma.h"
#include "panic.h"
#include "util/util.h"

DEFINE_LOGGER_STATIC(logger, "API Call Tracer");
#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&logger)

static struct thread_local_struct outputBufferKey;

static void cleanOutputBufferKey(void* buff) {
  buffer_free(buff);
}

static void helperCleanup() {
  thread_local_cleanup(&outputBufferKey);
}

static bool helperWorking = false;
static pthread_once_t helperInitControl = PTHREAD_ONCE_INIT;
static void helperInit() {
  int ret;
  if ((ret = thread_local_init(&outputBufferKey, cleanOutputBufferKey)) < 0) {
    pr_error("API call tracer disabled due error initializing: %d", ret);
    return;
  }
  
  helperWorking = true;
  atexit(helperCleanup);
  return;
}

static int initOutput() {
  pthread_once(&helperInitControl, helperInit);
  if (!helperWorking)
    return -ENOSYS;
  
  buffer_t* outputBuffer = thread_local_get(&outputBufferKey);
  if (outputBuffer)
    return 0;
  
  outputBuffer = buffer_new_with_size(64 * 1024);
  if (!outputBuffer) {
    pr_error("Out of memory while allocating output buffer");
    return -ENOMEM;
  }
  
  thread_local_set(&outputBufferKey, outputBuffer);
  return 0;
}

static int convertAndAppend(buffer_t* outputBuffer, const char* seperator, size_t count, enum debug_argument_type* types, va_list args) {
  int ret = 0;
  for (size_t i = 0; i < count; i++) {
    enum debug_argument_type type = types[i];
    int appendfRet = 0;
    switch (type) {
      case DEBUG_TYPE_FH_ARRAY: {
        fh_array* val = va_arg(args, fh_array*);
        appendfRet = buffer_appendf(outputBuffer, "%s", debug_get_unique_name(API_EXTERN(API_INTERN(val))));
        break;
      }
      case DEBUG_TYPE_FH_DESCRIPTOR: {
        fh_descriptor* val = va_arg(args, fh_descriptor*);
        appendfRet = buffer_appendf(outputBuffer, "%s#%" PRIu64, descriptor_get_name(API_INTERN(val)), API_INTERN(val)->foreverUniqueID);
        break;
      }
      case DEBUG_TYPE_FH_OBJECT: {
        fh_object* val = va_arg(args, fh_object*);
        appendfRet = buffer_appendf(outputBuffer, "%s", debug_get_unique_name(val));
        break;
      }
      case DEBUG_TYPE_FH_PARAM: {
        fh_param* val = va_arg(args, fh_param*);
        break;
      }
      case DEBUG_TYPE_FH_DMA_PTR: {
        fh_dma_ptr* val = va_arg(args, fh_dma_ptr*);
        struct dma_data* data = container_of(val, struct dma_data, apiPtr);
        appendfRet = buffer_appendf(outputBuffer, "DMA Ptr#%" PRIu64 " (at %p for object ID %" PRIu64 ")", data->foreverUniqueID, data->apiPtr.ptr, data->owningObjectID);
        break;
      }
      case DEBUG_TYPE_FH_CONTEXT: {
        fh_context* val = va_arg(args, fh_context*);
        appendfRet = buffer_appendf(outputBuffer, "Context#%" PRIu64, API_INTERN(val)->foreverUniqueID);
        break;
      }
      case DEBUG_TYPE_FLUFFYHEAP: {
        fluffyheap* val = va_arg(args, fluffyheap*);
        appendfRet = buffer_appendf(outputBuffer, "Heap#%" PRIu64 " (at 0x%p)", API_INTERN(val)->foreverUniqueID, val);
        break;
      }
      case DEBUG_TYPE_FH_DESCRIPTOR_LOADER: {
        fh_descriptor_loader val = va_arg(args, fh_descriptor_loader);
        appendfRet = buffer_appendf(outputBuffer, "Function at %p", val);
        break;
      }
      case DEBUG_TYPE_FH_DESCRIPTOR_PARAM:
      case DEBUG_TYPE_FH_DESCRIPTOR_PARAM_READONLY: {
        const fh_descriptor_param* val = va_arg(args, const fh_descriptor_param*);
        break;
      }
      case DEBUG_TYPE_FH_OBJECT_TYPE: {
        fh_object_type val = va_arg(args, fh_object_type);
        const char* stringified = descriptor_object_type_tostring(API_INTERN(val));
        if (stringified)
          appendfRet = buffer_append(outputBuffer, stringified);
        else
          appendfRet = buffer_appendf(outputBuffer, "<Unknown type %d>", val);
        break;
      }
      case DEBUG_TYPE_FH_TYPE_INFO_READONLY: {
        const fh_type_info* val = va_arg(args, const fh_type_info*);
        appendfRet = buffer_appendf(outputBuffer, "(fh_type_info) {.type = %s, .info", descriptor_object_type_tostring(API_INTERN(val->type)));
        if (appendfRet < 0)
          break;
        
        switch (val->type) {
          case FH_TYPE_NORMAL:
            appendfRet = buffer_appendf(outputBuffer, ".normal = %s", descriptor_get_name(API_INTERN(val->info.normal)));
            break;
          case FH_TYPE_ARRAY:
            appendfRet = buffer_appendf(outputBuffer, ".refArray = {.length = %zu, .elementDescriptor = %s}", val->info.refArray->length, descriptor_get_name(API_INTERN(val->info.refArray->elementDescriptor)));
            break;
          case FH_TYPE_COUNT:
            panic();
        }
        
        if (appendfRet < 0)
          break;
        
        appendfRet = buffer_append(outputBuffer, "}");
        break;
      }
      case DEBUG_TYPE_FH_MOD: {
        fh_mod val = va_arg(args, fh_mod);
        const char* stringified = api_mods_tostring(val);
        if (stringified)
          appendfRet = buffer_append(outputBuffer, stringified);
        else
          appendfRet = buffer_appendf(outputBuffer, "<Unknown mod %d>", val);
        break;
      }
      case DEBUG_TYPE_VOID_PTR_READWRITE: {
        void* val = va_arg(args, void*);
        appendfRet = buffer_appendf(outputBuffer, "(void*) %p", val);
        break;
      }
      case DEBUG_TYPE_VOID_PTR_READONLY: {
        const void* val = va_arg(args, const void*);
        appendfRet = buffer_appendf(outputBuffer, "(const void*) %p", val);
        break;
      }
      case DEBUG_TYPE_TIMESPEC_READONLY: {
        const struct timespec* val = va_arg(args, const struct timespec*);
        double secs = difftime(val->tv_sec, 0);
        long nanosecs = val->tv_nsec;
        appendfRet = buffer_appendf(outputBuffer, "(struct timespec) {.tv_sec = %lf, .tv_nsec = %ld}", secs, nanosecs);
        break;
      }
      case DEBUG_TYPE_CSTRING: {
        const char* val = va_arg(args, const char*);
        size_t len = strlen(val);
        
        // TODO: Make this configurable
        int maxLength = 48;
        appendfRet = buffer_appendf(outputBuffer, "\"%.*s\"", maxLength, val);
        if (len > maxLength && appendfRet >= 0)
          appendfRet = buffer_appendf(outputBuffer, " (truncated to %d from %zu)", maxLength, len);
        break;
      }
      case DEBUG_TYPE_BOOL: {
        // Why int?
        //
        // For a variadic function, the compiler doesn't know the types 
        // of the parameters corresponding to the , .... For historical
        // reasons, and to make the compiler's job easier, any
        // corresponding arguments of types narrower than int are promoted
        // to int or to unsigned int, and any arguments of type float
        // are promoted to double. (This is why printf uses the same
        // format specifiers for either float or double arguments.)
        //
        // https://stackoverflow.com/a/28054417/13447666
        bool val = va_arg(args, int);
        appendfRet = buffer_appendf(outputBuffer, "%s (0x%02x)", val ? "true" : "false", val);
        break;
      }
      case DEBUG_TYPE_INT: {
        int val = va_arg(args, int);
        appendfRet = buffer_appendf(outputBuffer, "%d", val);
        break;
      }
      case DEBUG_TYPE_LONG: {
        long val = va_arg(args, long);
        appendfRet = buffer_appendf(outputBuffer, "%ld", val);
        break;
      }
      case DEBUG_TYPE_ULONG: {
        unsigned long val = va_arg(args, unsigned long);
        appendfRet = buffer_appendf(outputBuffer, "%lu", val);
        break;
      }
      case DEBUG_TYPE_LONGLONG: {
        long long val = va_arg(args, long long);
        appendfRet = buffer_appendf(outputBuffer, "%lld", val);
        break;
      }
      case DEBUG_TYPE_ULONGLONG: {
        unsigned long long val = va_arg(args, unsigned long long);
        appendfRet = buffer_appendf(outputBuffer, "%llu", val);
        break;
      }
      case DEBUG_TYPE_VOID:
        break;
    }
    
    if (appendfRet < 0) {
      ret = appendfRet;
      goto failure;
    }

    if (i + 1 < count) {
      if ((ret = buffer_append(outputBuffer, seperator)) < 0)
        goto failure;
    }
  }
failure:
  return ret;
}

static const int MAX_NESTING = 1;
static thread_local int nestDepth = 0;

static thread_local double monotonicTimeStart = 0;
static thread_local double cpuTimeStart = 0;

void debug_helper_print_api_call(const char* source, enum debug_argument_type* argTypes, size_t argCount, ...) {
  // Don't print API call which were triggered inside API
  nestDepth++;
  if (nestDepth > MAX_NESTING)
    return;
  
  if (initOutput() < 0)
    return;
  if (!debug_can_do_api_tracing())
    return;
  
  buffer_t* outputBuffer = thread_local_get(&outputBufferKey);
  outputBuffer->data[0] = '\0';
  
  va_list arg;
  va_start(arg, argCount);
  int res = convertAndAppend(outputBuffer, ", ", argCount, argTypes, arg);
  va_end(arg);
  if (res < 0)
    goto failure;
  
  pr_debug("%s(%s)", source, buffer_string(outputBuffer));
failure:
  if (res < 0)
    pr_error("%s() called (Cannot stringify arguments: %d)", source, res);
  
  cpuTimeStart = util_get_thread_cpu_time();
  monotonicTimeStart = util_get_monotonic_time();
  return;
}

void debug_helper_process_return(const char* source, enum debug_argument_type type, ...) {
  if (nestDepth > MAX_NESTING) {
    nestDepth--;
    return;
  }
  nestDepth--;
  
  double cpuTimeTaken = util_get_thread_cpu_time() - cpuTimeStart;
  double monotonicTimeTaken = util_get_monotonic_time() - monotonicTimeStart;
  
  if (initOutput() < 0)
    return;
  if (!debug_can_do_api_tracing())
    return;
  
  buffer_t* outputBuffer = thread_local_get(&outputBufferKey);
  int res = 0;
  outputBuffer->data[0] = '\0';
  
  if (type == DEBUG_TYPE_VOID) {
    if ((res = buffer_append(outputBuffer, "(void)")) < 0)
      goto failure;
    goto void_type;
  }
  
  va_list arg;
  va_start(arg, argCount);
  res = convertAndAppend(outputBuffer, "", 1, &type, arg);
  va_end(arg);
  if (res < 0)
    goto failure;
  
void_type:
  pr_debug("... (from %s) = %s <Thread CPU: %lf Monotonic: %lf>", source, buffer_string(outputBuffer), cpuTimeTaken, monotonicTimeTaken);
failure:
  if (res < 0)
    pr_error("%s returned (Cannot stringify return value: %d) <Thread CPU: %lf Monotonic: %lf>", source, res, cpuTimeTaken, monotonicTimeTaken);
  if (monotonicTimeTaken < 0)
    BUG();
  if (cpuTimeTaken < 0)
    BUG();
  return;
}
