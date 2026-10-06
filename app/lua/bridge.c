#include "bridge.h"
#include "nearby.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <android/log.h>

static void bridge_error(LuaBridge *bridge,const char *message)
{
    snprintf(bridge->error,sizeof(bridge->error),"%s",message?message:"unknown Lua error");
    __android_log_print(ANDROID_LOG_ERROR,"YoungTableaux","%s",bridge->error);
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

static int layout_kind(const char *kind)
{
    if(!kind) return -1;
    if(!strcmp(kind,"label")) return SCRIPT_LABEL;
    if(!strcmp(kind,"separator")) return SCRIPT_SEPARATOR;
    if(!strcmp(kind,"field")) return SCRIPT_FIELD;
    if(!strcmp(kind,"shape")) return SCRIPT_SHAPE;
    if(!strcmp(kind,"output")) return SCRIPT_OUTPUT;
    if(!strcmp(kind,"hooks")) return SCRIPT_HOOKS;
    if(!strcmp(kind,"facts")) return SCRIPT_FACTS;
    if(!strcmp(kind,"wegert")) return SCRIPT_WEGERT;
    if(!strcmp(kind,"plot_controls")) return SCRIPT_PLOT_CONTROLS;
    return -1;
}

static void load_layout(LuaBridge *bridge,Console *console)
{
    if(bridge->layout_loaded || !bridge->ready) return;
    lua_State *state=bridge->state;
    lua_getglobal(state,"young_layout");
    if(!lua_isfunction(state,-1)) { lua_pop(state,1); return; }
    if(lua_pcall(state,0,1,0)!=LUA_OK) {
        bridge_error(bridge,lua_tostring(state,-1));
        lua_pop(state,1);
        return;
    }
    if(!lua_istable(state,-1)) { lua_pop(state,1); return; }

    ScriptLayoutItem items[SCRIPT_LAYOUT_MAX];
    int count=0;
    size_t length=lua_rawlen(state,-1);
    if(length>SCRIPT_LAYOUT_MAX) length=SCRIPT_LAYOUT_MAX;
    bool valid=true;
    for(size_t index=1;index<=length;++index) {
        lua_rawgeti(state,-1,(lua_Integer)index);
        if(!lua_istable(state,-1)) { lua_pop(state,1); valid=false; break; }

        lua_getfield(state,-1,"kind");
        const char *kind_text=lua_tostring(state,-1);
        int kind=layout_kind(kind_text);
        lua_pop(state,1);
        if(kind<0) { lua_pop(state,1); valid=false; break; }

        ScriptLayoutItem item={0};
        item.kind=(ScriptLayoutKind)kind;

        lua_getfield(state,-1,"arg");
        if(lua_isinteger(state,-1)) item.arg=(int)lua_tointeger(state,-1);
        lua_pop(state,1);

        lua_getfield(state,-1,"text");
        const char *text=lua_tostring(state,-1);
        snprintf(item.text,sizeof(item.text),"%s",text?text:"");
        lua_pop(state,1);

        lua_pop(state,1);
        items[count++]=item;
    }
    lua_pop(state,1);
    if(valid && count>0) {
        memcpy(console->script_layout,items,(size_t)count*sizeof(items[0]));
        console->script_layout_count=count;
        bridge->layout_loaded=true;
    }
}

void lua_bridge_init(LuaBridge *bridge,AAssetManager *assets)
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

static bool prepare_wegert(LuaBridge *bridge,Console *console,const Partition *partition)
{
    lua_State *state=bridge->state;
    lua_getglobal(state,"young_wegert");
    if(!lua_isfunction(state,-1)) { lua_pop(state,1); return false; }
    push_partition(state,partition);
    push_hooks(state,partition);
    if(lua_pcall(state,2,1,0)!=LUA_OK) {
        bridge_error(bridge,lua_tostring(state,-1));
        lua_pop(state,1);
        return false;
    }
    if(!lua_istable(state,-1)) { lua_pop(state,1); return false; }

    lua_getfield(state,-1,"n_lambda");
    int n_lambda=lua_isinteger(state,-1)?(int)lua_tointeger(state,-1):-1;
    lua_pop(state,1);
    lua_getfield(state,-1,"max_hook");
    int max_hook=lua_isinteger(state,-1)?(int)lua_tointeger(state,-1):-1;
    lua_pop(state,1);
    if(n_lambda<0 || max_hook<0 || max_hook>WEGERT_HOOK_MAX) {
        lua_pop(state,1);
        return false;
    }

    uint16_t counts[WEGERT_HOOK_MAX+1]={0};
    lua_getfield(state,-1,"hook_counts");
    if(!lua_istable(state,-1)) { lua_pop(state,2); return false; }
    bool valid=true;
    for(int hook=1;hook<=max_hook;++hook) {
        lua_rawgeti(state,-1,(lua_Integer)hook);
        if(lua_isnil(state,-1)) counts[hook]=0;
        else if(lua_isinteger(state,-1)) {
            lua_Integer value=lua_tointeger(state,-1);
            if(value<0 || value>UINT16_MAX) valid=false;
            else counts[hook]=(uint16_t)value;
        } else valid=false;
        lua_pop(state,1);
        if(!valid) break;
    }
    lua_pop(state,1);
    lua_pop(state,1);
    if(!valid) return false;

    console->wegert.n_lambda=n_lambda;
    console->wegert.max_hook=max_hook;
    memcpy(console->wegert.hook_counts,counts,sizeof(counts));
    console->wegert.valid=true;
    return true;
}

void lua_bridge_refresh(LuaBridge *bridge,Console *console)
{
    load_layout(bridge,console);
    if(strcmp(bridge->last_lambda,console->fields[SET_LAMBDA])==0 && console->scripted_facts[0]) return;
    snprintf(bridge->last_lambda,sizeof(bridge->last_lambda),"%s",console->fields[SET_LAMBDA]);

    if(!bridge->ready) {
        snprintf(console->scripted_facts,UI_TEXT,"SCHUR SPECIALIZATION\nFormula unavailable.");
        return;
    }

    Partition partition;
    MathStatus parsed=partition_parse(console->fields[SET_LAMBDA],&partition);
    if(parsed!=YT_OK) {
        snprintf(console->scripted_facts,UI_TEXT,"SCHUR SPECIALIZATION\nEnter a valid lambda first: %s",math_status(parsed));
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
        snprintf(console->scripted_facts,UI_TEXT,"SCHUR SPECIALIZATION\nFormula unavailable.");
        return;
    }
    push_partition(state,&partition);
    push_hooks(state,&partition);
    lua_pushstring(state,dimension);
    if(lua_pcall(state,3,1,0)!=LUA_OK) {
        bridge_error(bridge,lua_tostring(state,-1));
        snprintf(console->scripted_facts,UI_TEXT,"SCHUR SPECIALIZATION\nFormula unavailable.");
        lua_pop(state,1);
        return;
    }
    const char *facts=lua_tostring(state,-1);
    snprintf(console->scripted_facts,UI_TEXT,"%s",facts?facts:"SCHUR SPECIALIZATION\nNo output");
    lua_pop(state,1);

    (void)prepare_wegert(bridge,console,&partition);
}

static bool pull_partition(lua_State *state,int index,Partition *out)
{
    if(!lua_istable(state,index)) return false;
    size_t length=lua_rawlen(state,index);
    if(length>YT_DIM) return false;
    Partition p={0};
    int size=0;
    for(size_t i=1;i<=length;++i) {
        lua_rawgeti(state,index,(lua_Integer)i);
        if(!lua_isinteger(state,-1)) { lua_pop(state,1); return false; }
        lua_Integer value=lua_tointeger(state,-1);
        lua_pop(state,1);
        if(value<=0 || value>YT_DIM) return false;
        if(i>1 && value>p.rows[i-2]) return false;
        if(size>YT_CELLS-(int)value) return false;
        p.rows[i-1]=(int)value;
        size+=(int)value;
    }
    p.count=(int)length;
    *out=p;
    return true;
}

static bool same_partition(const Partition *a,const Partition *b)
{
    if(a->count!=b->count) return false;
    for(int row=0;row<a->count;++row) if(a->rows[row]!=b->rows[row]) return false;
    return true;
}

static bool replay_mirror(const Console *console,Partition *out)
{
    Partition p=console->shape_history_base;
    for(int i=0;i<console->shape_history_count;++i) {
        int row=console->shape_added_rows[i];
        if(row<0 || row>p.count || row>=YT_DIM) return false;
        if(row==p.count) p.rows[p.count++]=1;
        else ++p.rows[row];
    }
    *out=p;
    return true;
}

static void sync_mirror(Console *console,const Partition *current)
{
    Partition replayed;
    if(console->shape_history_count>0
       && replay_mirror(console,&replayed)
       && same_partition(&replayed,current)) return;
    console->shape_history_base=*current;
    console->shape_history_count=0;
}

static bool call_shape(LuaBridge *bridge,const char *name,const Partition *current,
                       const Cell *cell,Partition *out)
{
    lua_State *state=bridge->state;
    lua_getglobal(state,name);
    if(!lua_isfunction(state,-1)) { lua_pop(state,1); return false; }
    push_partition(state,current);
    int args=1;
    if(cell) {
        lua_pushinteger(state,(lua_Integer)cell->row);
        lua_pushinteger(state,(lua_Integer)cell->column);
        args=3;
    }
    if(lua_pcall(state,args,1,0)!=LUA_OK) {
        bridge_error(bridge,lua_tostring(state,-1));
        lua_pop(state,1);
        return false;
    }
    if(lua_isnil(state,-1)) { lua_pop(state,1); return false; }
    bool ok=pull_partition(state,-1,out);
    lua_pop(state,1);
    return ok;
}

static void install_partition(Console *console,Controls *ui,const Partition *partition)
{
    size_t used=0;
    console->fields[SET_LAMBDA][0]=0;
    for(int row=0;row<partition->count;++row) {
        int written=snprintf(console->fields[SET_LAMBDA]+used,512-used,
                             "%s%d",row?",":"",partition->rows[row]);
        if(written<0 || (size_t)written>=512-used) return;
        used+=(size_t)written;
    }
    if(!partition->count) snprintf(console->fields[SET_LAMBDA],512,"[]");
    console->scripted_facts[0]=0;
    console_run(console,DisplayPartition);
    console_run(console,ValidateTableau);
    ui->focus=0;
}

static bool find_addable(const Partition *partition,int row,Cell *out)
{
    if(row<0 || row>partition->count || row>=YT_DIM || partition_size(partition)>=YT_CELLS)
        return false;
    Cell cells[YT_DIM+1];
    int count=partition_addable(partition,cells);
    for(int i=0;i<count;++i) if(cells[i].row==row+1 && cells[i].column<=YT_DIM) {
        *out=cells[i];
        return true;
    }
    return false;
}

bool lua_bridge_shape_event(LuaBridge *bridge,Console *console,Controls *ui,ControlEvent event)
{
    if(!bridge->ready || event.kind!=EVENT_ACTIVATE) return false;
    int id=event.id;
    bool add=id>=ADDABLE_BASE && id<=ADDABLE_BASE+YT_DIM;
    if(!add && id!=SHAPE_UNDO && id!=SHAPE_RESET) return false;

    Partition current;
    if(partition_parse(console->fields[SET_LAMBDA],&current)!=YT_OK) return true;
    sync_mirror(console,&current);

    Partition expected=current, scripted;
    const char *function=NULL;
    Cell cell={0};

    if(add) {
        int row=id-ADDABLE_BASE;
        if(!find_addable(&current,row,&cell)) return true;
        function="young_shape_add";
        if(row==expected.count) expected.rows[expected.count++]=1;
        else ++expected.rows[row];
        if(!call_shape(bridge,function,&current,&cell,&scripted)
           || !same_partition(&scripted,&expected)) {
            bridge_error(bridge,"Lua click-add result failed C validation");
            return true;
        }
        if(!console->shape_history_count) console->shape_history_base=current;
        if(console->shape_history_count<YT_CELLS)
            console->shape_added_rows[console->shape_history_count++]=row;
    } else {
        if(!console->shape_history_count) return true;
        if(id==SHAPE_RESET) {
            function="young_shape_reset";
            expected=console->shape_history_base;
        } else {
            function="young_shape_undo";
            int row=console->shape_added_rows[console->shape_history_count-1];
            if(row==expected.count-1 && expected.rows[row]==1) --expected.count;
            else if(row>=0 && row<expected.count) --expected.rows[row];
            else return true;
        }
        if(!call_shape(bridge,function,&current,NULL,&scripted)
           || !same_partition(&scripted,&expected)) {
            bridge_error(bridge,"Lua undo/reset result failed C validation");
            return true;
        }
        if(id==SHAPE_RESET) console->shape_history_count=0;
        else --console->shape_history_count;
    }

    install_partition(console,ui,&scripted);
    snprintf(bridge->last_lambda,sizeof(bridge->last_lambda),"%s",console->fields[SET_LAMBDA]);
    bridge->last_lambda[0]=0;
    return true;
}

void lua_bridge_destroy(LuaBridge *bridge)
{
    if(bridge->state) lua_close(bridge->state);
    memset(bridge,0,sizeof(*bridge));
}
