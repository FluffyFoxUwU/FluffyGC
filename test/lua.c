#include "test/lua.h"

// Referenced function in lua.c
int runLuaInner (int argc, char **argv);
int runLua(int argc, char** argv) {
  return runLuaInner(argc, argv);
}

