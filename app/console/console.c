#include "console.h"
#include "nearby.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>
const OperationInfo operation_info[OP_COUNT]={
#define OP(symbol,section,label,input,output) [symbol]={label,input,output,section},
#include "operations.def"
#undef OP
};
static const char *sections[]={"1 PARTITION / YOUNG DIAGRAM","2 TABLEAU","3 HOOKS / CORNERS / CELLS","4 RSK","5 JEU DE TAQUIN","6 LITTLEWOOD-RICHARDSON","7 SYMMETRIC GROUP / REPRESENTATIONS","8 SYMMETRIC FUNCTIONS","9 YOUNG GRAPH / BRANCHING","10 RANDOM / ASYMPTOTIC","11 TYPE-A / COXETER","12 GLOBAL CONVENTIONS"};
static const char *tableau_kinds[]={"Standard","ArbitraryFilling","RowStandard","ColumnStandard","Semistandard","Skew","Shifted","Ribbon","Oscillating","KTableau"};
static const char *field_names[FIELD_COUNT]={"","lambda: rows","tableau: rows separated by ;","permutation","mu: partition","nu: partition","selected cell: row,column","word","biword: top ; bottom","matrix: rows separated by ;","n","alphabet maximum","weight / content","characteristic: 0 or p","prime p","Hecke parameter","basis","q","t","variables / specialization","coefficients","Young graph path","probability law","simple transposition word","second permutation","reading convention","left / right action"};
static void append(char *out,const char *format,...)
{
    size_t used=strlen(out); if(used>=UI_TEXT-1) return;
    va_list args; va_start(args,format); vsnprintf(out+used,UI_TEXT-used,format,args); va_end(args);
}
static void partition_text(char *out,const Partition *p)
{ append(out,"["); for(int r=0;r<p->count;++r) append(out,"%s%d",r?",":"",p->rows[r]); append(out,"]\n"); }
static void cells_text(char *out,const char *label,const Cell *cells,int n)
{ append(out,"%s: ",label); for(int i=0;i<n;++i) append(out,"(%d,%d) ",cells[i].row,cells[i].column); append(out,"\n"); }
static void tiles(TileProjection *out,const Partition *p,const Tableau *t)
{
    memset(out,0,sizeof(*out)); out->count=p->count; out->numbers=t!=NULL;
    for(int r=0;r<p->count;++r) { out->rows[r]=p->rows[r]; if(t) for(int col=0;col<p->rows[r];++col) out->values[r][col]=t->entries[r][col]; }
}
static void tableau_text(char *out,const char *label,const Tableau *t)
{
    append(out,"%s\n",label);
    if(!t->shape.count) append(out,"[]\n");
    for(int r=0;r<t->shape.count;++r) { for(int col=0;col<t->shape.rows[r];++col) append(out,"%d ",t->entries[r][col]); append(out,"\n"); }
}
static void refresh_partition(Console *c)
{
    Partition p; MathStatus s=partition_parse(c->fields[SET_LAMBDA],&p);
    c->partition_ok=s==YT_OK; c->output[0][0]=0; c->output[2][0]=0;
    double real=c->wegert.center_real, imag=c->wegert.center_imag;
    double half_height=c->wegert.half_height;
    memset(&c->wegert,0,sizeof(c->wegert));
    c->wegert.center_real=real; c->wegert.center_imag=imag;
    c->wegert.half_height=half_height>0.0?half_height:1.5;
    if(s!=YT_OK) { append(c->output[0],"%s\n",math_status(s)); append(c->output[2],"lambda: %s\n",math_status(s)); return; }
    Partition conjugate=partition_conjugate(&p);
    tiles(&c->partition,&p,NULL); tiles(&c->conjugate,&conjugate,NULL);
    append(c->output[0],"VALID partition\nsize |lambda| = %d\nconjugate = ",partition_size(&p)); partition_text(c->output[0],&conjugate);
    Cell cells[YT_CELLS+1]; int n=partition_cells(&p,cells); cells_text(c->output[0],"cells",cells,n);
    n=partition_removable(&p,cells); cells_text(c->output[2],"corners / removable",cells,n);
    n=partition_addable(&p,cells); cells_text(c->output[2],"addable",cells,n);
    Tableau h={0}; h.shape=p;
    c->wegert.valid=true;
    for(int r=0;r<p.count;++r) {
        c->wegert.n_lambda+=r*p.rows[r];
        for(int col=0;col<p.rows[r];++col) {
            int hook=partition_hook(&p,r+1,col+1);
            h.entries[r][col]=hook;
            if(hook>0 && hook<=WEGERT_HOOK_MAX) {
                ++c->wegert.hook_counts[hook];
                if(hook>c->wegert.max_hook) c->wegert.max_hook=hook;
            }
        }
    }
    tiles(&c->hooks,&p,&h); tableau_text(c->output[2],"hook lengths (rows)",&h);
    uint64_t value; s=partition_hook_product(&p,&value);
    if(s==YT_OK) append(c->output[2],"hook product = %" PRIu64 "\n",value); else append(c->output[2],"hook product: %s\n",math_status(s));
    s=partition_standard_count(&p,&value);
    if(s==YT_OK) append(c->output[2],"f^lambda = %" PRIu64 "\n",value); else append(c->output[2],"f^lambda: %s\n",math_status(s));
}
static void refresh_tableau(Console *c)
{
    Partition p; Tableau t; MathStatus a=partition_parse(c->fields[SET_LAMBDA],&p), b=tableau_parse(c->fields[SET_TABLEAU],&t);
    c->output[1][0]=0; c->tableau_ok=false;
    if(c->tableau_kind) {
        append(c->output[1],"NOT IMPLEMENTED\nValidation for %s\ninput: Tableau\noutput: Bool\nChoose Standard for v0.1 validation.",tableau_kinds[c->tableau_kind]);
        return;
    }
    if(a!=YT_OK || b!=YT_OK) { append(c->output[1],"%s: %s\n",a!=YT_OK?"lambda":"tableau",math_status(a!=YT_OK?a:b)); return; }
    Validation v=tableau_validate(&p,&t,c->decreasing);
    append(c->output[1],"shape compatible: %s\nrows strictly %s: %s\ncolumns strictly %s: %s\nstandard (1..n once): %s\n",v.shape?"YES":"NO",c->decreasing?"decreasing":"increasing",v.rows?"YES":"NO",c->decreasing?"decreasing":"increasing",v.columns?"YES":"NO",v.standard?"YES":"NO");
    tableau_text(c->output[1],"filling",&t); tiles(&c->tableau,&t.shape,&t); c->tableau_ok=true;
}
static void refresh_rsk(Console *c)
{
    c->output[3][0]=0; c->rsk_ok=false;
    if(c->insertion) { append(c->output[3],"NOT IMPLEMENTED\nColumnInsertion\nSelect RowInsertion for permutation RSK."); return; }
    Tableau p,q; MathStatus s=permutation_rsk(c->fields[SET_PERMUTATION],&p,&q);
    if(s!=YT_OK) { append(c->output[3],"permutation: %s",math_status(s)); return; }
    tableau_text(c->output[3],"P: insertion tableau",&p); tableau_text(c->output[3],"Q: recording tableau",&q);
    tiles(&c->p,&p.shape,&p); tiles(&c->q,&q.shape,&q); c->rsk_ok=true;
}
static void layout_add(Console *c,ScriptLayoutKind kind,int arg,const char *text)
{
    if(c->script_layout_count>=SCRIPT_LAYOUT_MAX) return;
    ScriptLayoutItem *item=&c->script_layout[c->script_layout_count++];
    item->kind=kind; item->arg=arg;
    snprintf(item->text,sizeof(item->text),"%s",text?text:"");
}
static void default_layout(Console *c)
{
    c->script_layout_count=0;
    layout_add(c,SCRIPT_LABEL,0,"YOUNG TABLEAUX 0.2.2");
    layout_add(c,SCRIPT_SEPARATOR,0,"");
    layout_add(c,SCRIPT_LABEL,0,"SHAPE / WEGERT");
    layout_add(c,SCRIPT_FIELD,SET_LAMBDA,"lambda: rows");
    layout_add(c,SCRIPT_SHAPE,0,"");
    layout_add(c,SCRIPT_LABEL,0,"WEGERT PLOT: s_lambda(1,z,z^2,...)");
    layout_add(c,SCRIPT_WEGERT,160,"");
    layout_add(c,SCRIPT_PLOT_CONTROLS,0,"");
    layout_add(c,SCRIPT_SEPARATOR,0,"");
    layout_add(c,SCRIPT_LABEL,0,"DERIVED FACTS");
    layout_add(c,SCRIPT_OUTPUT,0,"");
    layout_add(c,SCRIPT_OUTPUT,2,"");
    layout_add(c,SCRIPT_HOOKS,0,"hook cells");
    layout_add(c,SCRIPT_FACTS,0,"");
    layout_add(c,SCRIPT_SEPARATOR,0,"");
    layout_add(c,SCRIPT_LABEL,0,"MORE OPERATIONS / KITCHEN SINK");
}
void console_init(Console *c)
{
    memset(c,0,sizeof(*c));
    const char *defaults[FIELD_COUNT]={"","3,2,1","1,2,4;3,5;6","3,1,4,2","2,1","3,2,1","1,1","1,2,1","1,1,2;1,2,1","1,0;0,1","6","3","2,1","0","2","1","Schur","1","1","x1,x2","1","[]","Plancherel","1,2,1","1,2,3,4","RowReading","LeftAction"};
    for(int i=1;i<FIELD_COUNT;++i) snprintf(c->fields[i],sizeof(c->fields[i]),"%s",defaults[i]);
    for(int i=4;i<11;++i) snprintf(c->output[i],UI_TEXT,"Select an operation to inspect its input/output types.");
    snprintf(c->output[11],UI_TEXT,"English: top row longest.\nCells use one-based (row,column).\nContent default: column-row.\nPermutation list is one-line notation.\nReading and group action controls are inventory only.\nNo semistandard/skew/shifted validation in v0.1.");
    snprintf(c->scripted_facts,UI_TEXT,"SCHUR SPECIALIZATION\nLoading...");
    default_layout(c);
    refresh_partition(c); refresh_tableau(c); refresh_rsk(c);
}
void console_run(Console *c,Operation op)
{
    if(op<0 || op>=OP_COUNT) return;
    int section=operation_info[op].section;
    switch(op) {
    case DisplayPartition: case ConjugatePartition: case ListCells:
    case ComputeHookLengths: case ComputeHookProduct: case CountStandardTableaux:
    case FindCorners: case FindAddableCells: case FindRemovableCells: refresh_partition(c); return;
    case DisplayTableau: case ValidateTableau: refresh_tableau(c); return;
    case RSKPermutation: refresh_rsk(c); return;
    case RepresentationDimension: {
        Partition p; uint64_t count; MathStatus s=partition_parse(c->fields[SET_LAMBDA],&p);
        if(s==YT_OK) s=partition_standard_count(&p,&count);
        c->output[section][0]=0;
        if(strcmp(c->fields[SET_CHARACTERISTIC],"0")) append(c->output[section],"NOT IMPLEMENTED\nCharacteristicP representation dimension\ninput: Partition\noutput: Integer");
        else if(s==YT_OK) append(c->output[section],"Characteristic zero Specht dimension = %" PRIu64,count);
        else append(c->output[section],"%s",math_status(s));
        return;
    }
    default: break;
    }
    snprintf(c->output[section],UI_TEXT,"NOT IMPLEMENTED\n%s\ninput: %s\noutput: %s",operation_info[op].label,operation_info[op].input,operation_info[op].output);
    if(section==1) c->tableau_ok=false;
    if(section==3) c->rsk_ok=false;
}
static void field(Console *c,Controls *u,int id)
{ controls_add(u,0,LABEL,field_names[id],0,NULL); controls_add(u,id,FIELD,c->fields[id],0,NULL); }
static void diagram(Controls *u,const char *label,const TileProjection *p)
{ controls_add(u,0,LABEL,label,0,NULL); controls_add(u,0,DIAGRAM,"",(p->count?p->count:1)*12*u->scale+8*u->scale,p); }
static void scripted_top(Console *c,Controls *u)
{
    for(int i=0;i<c->script_layout_count;++i) {
        const ScriptLayoutItem *item=&c->script_layout[i];
        switch(item->kind) {
        case SCRIPT_LABEL:
            controls_add(u,0,LABEL,item->text,0,NULL);
            break;
        case SCRIPT_SEPARATOR:
            controls_add(u,0,SEPARATOR,"",2*u->scale,NULL);
            break;
        case SCRIPT_FIELD:
            if(item->text[0]) controls_add(u,0,LABEL,item->text,0,NULL);
            if(item->arg>0 && item->arg<FIELD_COUNT)
                controls_add(u,item->arg,FIELD,c->fields[item->arg],0,NULL);
            break;
        case SCRIPT_SHAPE:
            if(c->partition_ok) nearby_shape(c,u);
            break;
        case SCRIPT_OUTPUT:
            if(item->arg>=0 && item->arg<12) controls_add(u,0,OUTPUT,c->output[item->arg],0,NULL);
            break;
        case SCRIPT_HOOKS:
            if(c->partition_ok) diagram(u,item->text[0]?item->text:"hook cells",&c->hooks);
            break;
        case SCRIPT_FACTS:
            controls_add(u,0,OUTPUT,c->scripted_facts,0,NULL);
            break;
        case SCRIPT_WEGERT:
            if(c->partition_ok) controls_add(u,0,WEGERT,"",(item->arg>0?item->arg:160)*u->scale,&c->wegert);
            break;
        case SCRIPT_PLOT_CONTROLS:
            if(c->partition_ok) nearby_plot_controls(c,u);
            break;
        }
    }
}
void console_layout(Console *c,Controls *u,int w,int h)
{
    controls_begin(u,w,h,u->focus!=0);
    char total[16]="?";
    Partition total_partition;
    if(partition_parse(c->fields[SET_LAMBDA],&total_partition)==YT_OK)
        snprintf(total,sizeof(total),"%d",partition_size(&total_partition));
    controls_add(u,0,HERO,total,24*u->scale,NULL);
    controls_add(u,0,LABEL,"YOUNG TABLEAUX 0.2.2",0,NULL);
    scripted_top(c,u);

    for(int s=0;s<12;++s) {
        controls_add(u,0,SEPARATOR,"",2*u->scale,NULL); controls_add(u,0,LABEL,sections[s],0,NULL);
        switch(s) {
        case 0: controls_add(u,0,LABEL,"Uses lambda from the top of the screen.",0,NULL); break;
        case 1: field(c,u,SET_TABLEAU); field(c,u,SET_ALPHABET); field(c,u,SET_WEIGHT);
            {
                char kind_label[96]; snprintf(kind_label,sizeof(kind_label),"Tableau kind: %s",tableau_kinds[c->tableau_kind]);
                controls_add(u,CHOOSE_TABLEAU_KIND,CHOICE,kind_label,0,NULL);
            }
            controls_add(u,CHOOSE_STANDARD,CHOICE,c->decreasing?"Standard convention: decreasing":"Standard convention: increasing",0,NULL); break;
        case 2: field(c,u,SET_CELL); controls_add(u,0,LABEL,"Hook facts and diagram are shown near the top.",0,NULL); break;
        case 3:
            controls_add(u,0,LABEL,"Ordinary permutation RSK: insert left to right. RowInsertion bumps first strictly greater entry; Q records insertion step in the new cell. P,Q increase along rows and down columns.",0,NULL);
            field(c,u,SET_PERMUTATION); field(c,u,SET_WORD); field(c,u,SET_BIWORD); field(c,u,SET_MATRIX);
            controls_add(u,CHOOSE_INSERTION,CHOICE,c->insertion?"ColumnInsertion: not implemented":"RowInsertion: implemented",0,NULL); break;
        case 4: controls_add(u,0,LABEL,"Uses tableau and selected cell above.",0,NULL); break;
        case 5: controls_add(u,0,LABEL,"Uses lambda above; LR uses outer nu / inner lambda and content mu.",0,NULL); field(c,u,SET_MU); field(c,u,SET_NU); break;
        case 6: field(c,u,SET_CHARACTERISTIC); field(c,u,SET_PRIME); field(c,u,SET_HECKE); controls_add(u,0,LABEL,"Uses lambda and permutation above. Only characteristic-zero dimension is implemented.",0,NULL); break;
        case 7: field(c,u,SET_BASIS); field(c,u,SET_COEFFICIENTS); field(c,u,SET_VARIABLES); field(c,u,SET_Q); field(c,u,SET_T); break;
        case 8: field(c,u,SET_PATH); controls_add(u,0,LABEL,"Start lambda, end nu; paths encode box additions.",0,NULL); break;
        case 9: field(c,u,SET_N); controls_add(u,STEP_MINUS,BUTTON,"n - 1",0,NULL); controls_add(u,STEP_PLUS,BUTTON,"n + 1",0,NULL); field(c,u,SET_LAW); break;
        case 10: field(c,u,SET_COXETER); field(c,u,SET_SECOND_PERMUTATION); break;
        case 11:
            controls_add(u,CHOOSE_ORIENTATION,CHOICE,c->french?"Diagram orientation: French":"Diagram orientation: English",0,NULL);
            controls_add(u,CHOOSE_CONTENT,CHOICE,c->content_convention?"Content: row-column (inventory)":"Content: column-row (inventory)",0,NULL);
            field(c,u,SET_READING); field(c,u,SET_ACTION); break;
        }
        for(int op=0;op<OP_COUNT;++op) if(operation_info[op].section==s) controls_add(u,OP_BASE+op,BUTTON,operation_info[op].label,0,NULL);
        if(s!=0 && s!=2) controls_add(u,0,OUTPUT,c->output[s],0,NULL);
        if(s==1 && c->tableau_ok) diagram(u,"filling cells",&c->tableau);
        if(s==3 && c->rsk_ok) { diagram(u,"P cells",&c->p); diagram(u,"Q cells",&c->q); }
    }
    controls_end(u);
}
void console_event(Console *c,Controls *u,ControlEvent e)
{
    if(e.kind==EVENT_NONE) return;
    if(e.kind==EVENT_FOCUS) return;
    if(nearby_event(c,u,e)) return;
    if(e.id>=OP_BASE) { console_run(c,(Operation)(e.id-OP_BASE)); return; }
    switch(e.id) {
    case CHOOSE_ORIENTATION: c->french=!c->french; break;
    case CHOOSE_STANDARD: c->decreasing=!c->decreasing; refresh_tableau(c); break;
    case CHOOSE_INSERTION: c->insertion=!c->insertion; refresh_rsk(c); break;
    case CHOOSE_CONTENT: c->content_convention=!c->content_convention; break;
    case CHOOSE_TABLEAU_KIND: c->tableau_kind=(c->tableau_kind+1)%10; refresh_tableau(c); break;
    case STEP_MINUS: case STEP_PLUS: {
        char *end; long n=strtol(c->fields[SET_N],&end,10);
        if(!*end && n>=0 && n<1000000) { if(e.id==STEP_PLUS) ++n; else if(n) --n; snprintf(c->fields[SET_N],512,"%ld",n); }
        else snprintf(c->output[9],UI_TEXT,"INVALID n: stepper range 0..999999");
        break;
    }
    default: break;
    }
    (void)u;
}
static const char *key_labels[]={"1","2","3","4","5","6","7","8","9","0",",",";","-","SPACE","DEL","CLEAR","[","]",".","DONE"};
const char *console_key_label(int key) { return key>=0 && key<20?key_labels[key]:""; }
int console_key_hit(const Controls *u,int x,int y,int height)
{
    int top=height-90*u->scale, cell_h=20*u->scale;
    if(x<0 || x>=u->width || y<top+5*u->scale || y>=top+85*u->scale) return -1;
    return ((y-top-5*u->scale)/cell_h)*5 + x*5/u->width;
}
void console_key(Console *c,Controls *u,int key)
{
    if(!u->focus || u->focus>=FIELD_COUNT || key<0 || key>=20) return;
    char *text=c->fields[u->focus]; size_t len=strlen(text);
    if(key==19) { u->focus=0; return; }
    if(key==14) { if(len) text[len-1]=0; }
    else if(key==15) text[0]=0;
    else {
        const char *add=key==13?" ":key_labels[key]; size_t length=strlen(add);
        if(len+length<512) memcpy(text+len,add,length+1);
        else { snprintf(c->output[11],UI_TEXT,"INPUT LIMIT: 511 characters"); return; }
    }
    if(u->focus==SET_LAMBDA) {
        c->shape_history_count=0;
        refresh_partition(c); refresh_tableau(c); c->scripted_facts[0]=0;
    }
    if(u->focus==SET_TABLEAU) refresh_tableau(c);
    if(u->focus==SET_PERMUTATION) refresh_rsk(c);
}
