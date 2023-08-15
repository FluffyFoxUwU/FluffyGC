#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

#include "logger/logger.h"
#include "panic.h"

#if IS_ENABLED(CONFIG_BUILD_ASAN)
const char* __asan_default_options() {
  return CONFIG_BUILD_ASAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_BUILD_UBSAN)
const char* __ubsan_default_options() {
  return CONFIG_BUILD_UBSAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_BUILD_TSAN)
const char* __tsan_default_options() {
  return CONFIG_BUILD_TSAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_BUILD_MSAN)
const char* __msan_default_options() {
  return CONFIG_BUILD_MSAN_OPTS;
}
#endif

#if IS_ENABLED(CONFIG_BUILD_LLVM_XRAY)
static const char* xrayOpts = CONFIG_BUILD_LLVM_XRAY_OPTS;
#else
static const char* xrayOpts = "";
#endif

DEFINE_LOGGER_STATIC(logger, "XRay Settings");
#undef LOGGER_DEFAULT
#define LOGGER_DEFAULT (&logger)

static void selfRestart(char** argv) {
  int err = setenv("XRAY_OPTIONS", xrayOpts, true);
  int errNum = errno;
  if (err < 0) {
    pr_fatal("Failed to set XRAY_OPTIONS: setenv: %s", strerror(errNum));
    exit(EXIT_FAILURE);
  }
  
  err = execvp(argv[0], argv);
  if (err < 0) {
    pr_fatal("Failed to self restart: execvp: %s", strerror(errNum));
    exit(EXIT_FAILURE);
  }
  
  hard_panic("Can't reached here!!!");
}

void special_premain(int argc, char** argv) {
  if (!IS_ENABLED(CONFIG_BUILD_LLVM_XRAY))
    return;

  const char* var;
  if ((var = getenv("XRAY_OPTIONS"))) {
    pr_info("Success XRAY_OPTIONS now contain \"%s\"\n", var);
    return;
  }

  pr_alert("Self restarting due XRAY_OPTIONS not set\n", stderr);
  selfRestart(argv);
}


