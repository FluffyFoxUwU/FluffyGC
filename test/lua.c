#include "test/lua.h"

#include "../FluffyGCLuaPort/onelua.c"

int runLua(int argc, char** argv) {
  return runLuaInner(argc, argv);
}

