#include "bridge.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static void bridge_error(LuaBridge *bridge, const char *message)
{
    snprintf(bridge->error,sizeof(bridge->error),"%s",message?message:"unknown Lua error");
}

void lua_bridge_init(LuaBridge *bridge, AAssetManager *assets)
{
    memset(bridge,0,sizeof(*bridge));
    if(!assets) { bridge_error(bridge,"Android asset manager unavailable"); return; }
    AAsset *asset=AAssetManager_open(assets,"facts.lua",AASSET_MODE_STREAMING);
    if(!asset) { bridge_error(bridge,"facts.lua asset missing"); return; }
    off_t raw_length=AAsset_getLength(asset);
    if(raw_length<=0) { AAsset_close(asset); bridge_error(bridge,"facts.lua asset empty"); return; }
    size_t length=(size_t)raw_length;
    char *source=malloc(length);
    if(!source) { AAsset_close(asset); bridge_error(bridge,"out of memory loading facts.lua"); return; }
    size_t used=0;
    while(used<length) {
        int got=AAsset_read(asset,source+used,length-used);
        if(got<=0) break;
        used+=(size_t)got;
    }
    AAsset_close(asset);
    if(used!=length) { free(source); bridge_error(bridge,"short read loading facts.lua"); return; }

    lua_State *state=luaL_newstate();
    if(!state) { free(source); bridge_error(bridge,"luaL_newstate failed"); return; }
    luaL_openlibs(state);
    int status=luaL_loadbuffer(state,source,length,"facts.lua");
    free(source);
    if(status==LUA_OK) status=lua_pcall(state,0,0,0);
    if(status!=LUA_OK) {
        bridge_error(bridge,lua_tostring(state,-1));
        lua_close(state);
        return;
    }
    bridge->state=state;
    bridge->ready=true;
}

static void push_partition(lua_State *state,const Partition *partition)
{
    lua_createtable(state,partition->count,0);
    for(int row=0;row<partition->count;++row) {
        lua_pushinteger(state,(lua_Integer)partition->rows[row]);
        lua_rawseti(state,-2,(lua_Integer)row+1);
    }
}

static void push_hooks(lua_State *state,const Partition *partition)
{
    int count=partition_size(partition);
    lua_createtable(state,count,0);
    int index=1;
    for(int row=1;row<=partition->count;++row)
        for(int column=1;column<=partition->rows[row-1];++column) {
            lua_pushinteger(state,(lua_Integer)partition_hook(partition,row,column));
            lua_rawseti(state,-2,(lua_Integer)index++);
        }
}

void lua_bridge_refresh(LuaBridge *bridge, Console *console)
{
    if(strcmp(bridge->last_lambda,console->fields[SET_LAMBDA])==0 && console->scripted_facts[0]) return;
    snprintf(bridge->last_lambda,sizeof(bridge->last_lambda),"%s",console->fields[SET_LAMBDA]);

    if(!bridge->ready) {
        snprintf(console->scripted_facts,UI_TEXT,"SCRIPTED FACTS (LUA)\nUnavailable: %s",bridge->error[0]?bridge->error:"not initialized");
        return;
    }

    Partition partition;
    MathStatus parsed=partition_parse(console->fields[SET_LAMBDA],&partition);
    if(parsed!=YT_OK) {
        snprintf(console->scripted_facts,UI_TEXT,"SCRIPTED FACTS (LUA)\nEnter a valid lambda first: %s",math_status(parsed));
        return;
    }

    uint64_t standard_count=0;
    MathStatus counted=partition_standard_count(&partition,&standard_count);
    char dimension[64];
    if(counted==YT_OK) snprintf(dimension,sizeof(dimension),"%" PRIu64,standard_count);
    else snprintf(dimension,sizeof(dimension),"%s",math_status(counted));

    lua_State *state=bridge->state;
    lua_getglobal(state,"young_facts");
    if(!lua_isfunction(state,-1)) {
        lua_pop(state,1);
        snprintf(console->scripted_facts,UI_TEXT,"SCRIPTED FACTS (LUA)\nyoung_facts() missing");
        return;
    }
    push_partition(state,&partition);
    push_hooks(state,&partition);
    lua_pushstring(state,dimension);
    if(lua_pcall(state,3,1,0)!=LUA_OK) {
        const char *message=lua_tostring(state,-1);
        snprintf(console->scripted_facts,UI_TEXT,"SCRIPTED FACTS (LUA)\n%s",message?message:"evaluation error");
        lua_pop(state,1);
        return;
    }
    const char *facts=lua_tostring(state,-1);
    snprintf(console->scripted_facts,UI_TEXT,"%s",facts?facts:"SCRIPTED FACTS (LUA)\nNo output");
    lua_pop(state,1);
}

void lua_bridge_destroy(LuaBridge *bridge)
{
    if(bridge->state) lua_close(bridge->state);
    memset(bridge,0,sizeof(*bridge));
}
