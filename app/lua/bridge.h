#ifndef YOUNG_LUA_BRIDGE_H
#define YOUNG_LUA_BRIDGE_H
#include <stdbool.h>
#include <android/asset_manager.h>
#include "console.h"

struct lua_State;

typedef struct {
    struct lua_State *state;
    bool ready;
    char error[256];
    char last_lambda[512];
} LuaBridge;

void lua_bridge_init(LuaBridge *bridge, AAssetManager *assets);
void lua_bridge_refresh(LuaBridge *bridge, Console *console);
void lua_bridge_destroy(LuaBridge *bridge);

#endif
