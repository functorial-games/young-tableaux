#include <stdio.h>
#include <string.h>
#include <stdbool.h>
static const struct { const char *name,*input,*output; } operations[] = {
#define OP(symbol,section,label,input,output) {#symbol,input,output},
#include "operations.def"
#undef OP
};
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
    if(valid) puts("PASS every executable operation has its exact sketch input and output");
    return valid?0:1;
}
