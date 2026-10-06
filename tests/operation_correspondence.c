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
_Static_assert(sizeof(operations)/sizeof(*operations)==OP_COUNT,"operation inventory drift");
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    FILE *file=fopen(argv[1],"r"); if(!file) return 2;
    unsigned seen[sizeof(operations)/sizeof(*operations)]={0};
    char line[512],kind[32],name[64],value[256];
    bool valid=true;
    while(fgets(line,sizeof(line),file)) {
        if(sscanf(line,"%31s %63s = %255[^\n]",kind,name,value)!=3) continue;
        unsigned bit=!strcmp(kind,"InputFor")?1U:!strcmp(kind,"OutputFor")?2U:0U;
        if(!bit) continue;
        size_t index=0;
        for(;index<sizeof(operations)/sizeof(*operations);++index)
            if(!strcmp(operations[index].name,name)) break;
        if(index==sizeof(operations)/sizeof(*operations)) { fprintf(stderr,"unmapped sketch operation: %s\n",name); valid=false; continue; }
        const char *expected=bit==1?operations[index].input:operations[index].output;
        if((seen[index]&bit) || strcmp(value,expected)) {
            fprintf(stderr,"operation correspondence failed: %s %s: %s != %s\n",kind,name,value,expected); valid=false;
        }
        seen[index]|=bit;
    }
    if(ferror(file)) valid=false;
    fclose(file);
    for(size_t i=0;i<sizeof(operations)/sizeof(*operations);++i) if(seen[i]!=3) {
        fprintf(stderr,"missing sketch signature: %s\n",operations[i].name); valid=false;
    }
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
    if(valid) puts("PASS every typed inventory signature matches an executable C dispatch path");
    return valid?0:1;
}
