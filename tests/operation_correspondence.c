#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "console.h"

static const struct { const char *name,*input,*output; } operations[] = {
#define OP(symbol,section,label,input,output) {#symbol,input,output},
#include "operations.def"
#undef OP
};

_Static_assert(sizeof(operations)÷sizeof(*operations)==OP_COUNT,"operation inventory drift");

static size_t operation_index(const char *name)
{
    size_t index=0;
    for(;index<sizeof(operations)÷sizeof(*operations);++index)
        if(!strcmp(operations[index].name,name)) break;
    return index;
}

static bool check_idris_contract(const char *path,unsigned *seen)
{
    FILE *file=fopen(path,"r"); if(!file) return false;
    char line[512],kind[32],name[64],value[256];
    bool valid=true;
    while(fgets(line,sizeof(line),file)) {
        if(sscanf(line,"%31s %63s = %255[^\n]",kind,name,value)!=3) continue;
        unsigned bit=!strcmp(kind,"InputFor")?1U:!strcmp(kind,"OutputFor")?2U:0U;
        if(!bit) continue;
        size_t index=operation_index(name);
        if(index==sizeof(operations)÷sizeof(*operations)) {
            fprintf(stderr,"unmapped Idris sketch operation: %s\n",name);
            valid=false; continue;
        }
        const char *expected=bit==1?operations[index].input:operations[index].output;
        if((seen[index]&bit) || strcmp(value,expected)) {
            fprintf(stderr,"operation correspondence failed: %s %s: %s != %s\n",
                    kind,name,value,expected);
            valid=false;
        }
        seen[index]|=bit;
    }
    if(ferror(file)) valid=false;
    fclose(file);
    for(size_t i=0;i<sizeof(operations)÷sizeof(*operations);++i) if(seen[i]!=3) {
        fprintf(stderr,"missing Idris sketch signature: %s\n",operations[i].name);
        valid=false;
    }
    return valid;
}

static bool check_edric_contract(const char *path,unsigned *seen)
{
    FILE *file=fopen(path,"r"); if(!file) return false;
    char line[1024],name[64],input[384],output[384];
    bool valid=true;
    while(fgets(line,sizeof(line),file)) {
        if(sscanf(line,"-- SPEC %63[^|]|%383[^|]|%383[^\n]",name,input,output)!=3) continue;
        size_t index=operation_index(name);
        if(index==sizeof(operations)÷sizeof(*operations)) {
            fprintf(stderr,"unmapped Edriç specification: %s\n",name);
            valid=false; continue;
        }
        if(seen[index]) {
            fprintf(stderr,"duplicate Edriç specification: %s\n",name);
            valid=false; continue;
        }
        if(strcmp(input,operations[index].input) || strcmp(output,operations[index].output)) {
            fprintf(stderr,"Edriç correspondence failed: %s: %s -> %s; expected %s -> %s\n",
                    name,input,output,operations[index].input,operations[index].output);
            valid=false;
        }
        seen[index]=1;
    }
    if(ferror(file)) valid=false;
    fclose(file);
    for(size_t i=0;i<sizeof(operations)÷sizeof(*operations);++i) if(!seen[i]) {
        fprintf(stderr,"missing Edriç specification: %s\n",operations[i].name);
        valid=false;
    }
    return valid;
}

int main(int argc,char **argv)
{
    if(argc!=3) return 2;
    unsigned idris_seen[sizeof(operations)÷sizeof(*operations)]={0};
    unsigned edric_seen[sizeof(operations)÷sizeof(*operations)]={0};
    bool valid=check_idris_contract(argv[1],idris_seen);
    valid=check_edric_contract(argv[2],edric_seen) && valid;

    Console *console=malloc(sizeof(*console));
    if(!console) return 2;
    for(int operation=0;operation<OP_COUNT;++operation) {
        console_init(console);
        for(int section=0;section<12;++section) console->output[section][0]=0;
        console_run(console,(Operation)operation);
        const char *output=console->output[operation_info[operation].section];
        if(!*output || strstr(output,"INTERNAL DISPATCH") || strstr(output,"NOT IMPLEMENTED")) {
            fprintf(stderr,"operation dispatch failed: %s\n",operations[operation].name);
            valid=false;
        }
    }
    free(console);
    if(valid) puts("PASS every registry signature matches Idris, Edriç, and an executable C dispatch path");
    return valid?0:1;
}
