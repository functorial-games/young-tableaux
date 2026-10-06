#include "console.h"
#include "paint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(condition) do { ++checks; if(!(condition)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); exit(1); } } while(0)
static unsigned checks;
static Partition partition(const char *text)
{ Partition p; CHECK(partition_parse(text,&p)==YT_OK); return p; }
static bool equal_partition(Partition a,Partition b)
{ if(a.count!=b.count) return false; for(int r=0;r<a.count;++r) if(a.rows[r]!=b.rows[r]) return false; return true; }
/* Independent branching recurrence: remove the largest entry of an SYT. */
static uint64_t count_by_branching(Partition p)
{
    if(!p.count) return 1;
    uint64_t count=0;
    for(int r=0;r<p.count;++r) if(r+1==p.count || p.rows[r]>p.rows[r+1]) {
        Partition smaller=p; --smaller.rows[r]; if(!smaller.rows[r]) --smaller.count;
        count+=count_by_branching(smaller);
    }
    return count;
}
static void check_all_partitions(Partition p,int remaining,int maximum)
{
    if(!remaining) {
        uint64_t value; CHECK(partition_standard_count(&p,&value)==YT_OK);
        CHECK(value==count_by_branching(p));
        CHECK(equal_partition(partition_conjugate(&(Partition){0}),(Partition){0}));
        Partition conjugate=partition_conjugate(&p);
        CHECK(equal_partition(partition_conjugate(&conjugate),p));
        return;
    }
    for(int row=remaining<maximum?remaining:maximum;row>=1;--row) {
        Partition next=p; next.rows[next.count++]=row;
        check_all_partitions(next,remaining-row,row);
    }
}
static void partitions(void)
{
    Partition p=partition("[4, 2, 1]"); CHECK(partition_size(&p)==7);
    CHECK(equal_partition(partition_conjugate(&p),partition("3,2,1,1")));
    CHECK(equal_partition(partition_conjugate(&(Partition){0}),(Partition){0}));
    CHECK(equal_partition(partition_conjugate(&(Partition){3,{3,2,1}}),partition("3,2,1")));
    const char *invalid[]={"1,2","0","-1","2,0","[3,2","3,2]","2,,1","1,","[1,]","hello","1x","[1]garbage","1;1","+","--2","1.5","1-1"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i) CHECK(partition_parse(invalid[i],&p)==YT_MALFORMED);
    CHECK(partition_parse("99999999999999999999999999",&p)==YT_OVERFLOW);
    CHECK(partition_parse("65",&p)==YT_LIMIT);
    CHECK(partition_parse("64,64,64,64,1",&p)==YT_LIMIT);
    p=partition("3,2,1"); Cell cells[YT_CELLS+1]; int n=partition_removable(&p,cells);
    CHECK(n==3); CHECK(cells[0].row==1 && cells[0].column==3); CHECK(cells[2].row==3 && cells[2].column==1);
    n=partition_addable(&p,cells); CHECK(n==4); CHECK(cells[0].row==1 && cells[0].column==4); CHECK(cells[3].row==4 && cells[3].column==1);
    p=partition("2,2"); CHECK(partition_removable(&p,cells)==1); CHECK(cells[0].row==2 && cells[0].column==2);
    CHECK(partition_addable(&p,cells)==2); CHECK(cells[1].row==3 && cells[1].column==1);
    CHECK(partition_cells(&p,cells)==4); CHECK(cells[2].row==2 && cells[2].column==1);
    p=partition("[]"); CHECK(partition_removable(&p,cells)==0); CHECK(partition_addable(&p,cells)==1); CHECK(cells[0].row==1 && cells[0].column==1);
    p=partition("3,2,1"); int hooks[3][3]={{5,3,1},{3,1,0},{1,0,0}};
    for(int r=1;r<=3;++r) for(int c=1;c<=p.rows[r-1];++c) CHECK(partition_hook(&p,r,c)==hooks[r-1][c-1]);
    CHECK(partition_hook(&p,0,1)==0); CHECK(partition_hook(&p,1,4)==0);
    uint64_t value=0; CHECK(partition_hook_product(&p,&value)==YT_OK && value==45);
    struct { const char *shape; uint64_t count; } known[]={ {"[]",1},{"1",1},{"4",1},{"1,1,1,1",1},{"2,1",2},{"2,2",2},{"3,2",5},{"3,2,1",16},{"4,2,1",35},{"4,3,2,1",768},{"4,4,4",462},{"20,20",6564120420ULL},{"64",1} };
    for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);++i) { p=partition(known[i].shape); CHECK(partition_standard_count(&p,&value)==YT_OK); CHECK(value==known[i].count); }
    p=partition("64"); value=77; CHECK(partition_hook_product(&p,&value)==YT_OVERFLOW && value==77);
    p=partition("32,32"); CHECK(partition_standard_count(&p,&value)==YT_OK && value==55534064877048198ULL);
    p=partition("64,64"); value=88; CHECK(partition_standard_count(&p,&value)==YT_OVERFLOW && value==88);
    for(int size=0;size<=10;++size) check_all_partitions((Partition){0},size,size);
}
static void tableaux(void)
{
    Partition p=partition("3,2,1"); Tableau t;
    CHECK(tableau_parse("1,2,4;3,5;6",&t)==YT_OK);
    Validation v=tableau_validate(&p,&t,false); CHECK(v.shape && v.rows && v.columns && v.standard);
    CHECK(!tableau_validate(&p,&t,true).standard);
    CHECK(tableau_parse("6,5,3;4,2;1",&t)==YT_OK); CHECK(tableau_validate(&p,&t,true).standard);
    CHECK(tableau_parse("1,2,2;3,5;6",&t)==YT_OK); v=tableau_validate(&p,&t,false); CHECK(v.shape && !v.rows && !v.standard);
    CHECK(tableau_parse("3,4,6;1,5;2",&t)==YT_OK); v=tableau_validate(&p,&t,false); CHECK(v.rows && !v.columns && !v.standard);
    CHECK(tableau_parse("1,2;3,4",&t)==YT_OK); CHECK(!tableau_validate(&p,&t,false).shape);
    CHECK(tableau_parse("1,2,7;3,5;6",&t)==YT_OK); CHECK(!tableau_validate(&p,&t,false).standard);
    CHECK(tableau_parse("1,2,4;3,5;6;",&t)==YT_MALFORMED);
    CHECK(tableau_parse("1;;2",&t)==YT_MALFORMED); CHECK(tableau_parse("1,x",&t)==YT_MALFORMED);
    CHECK(tableau_parse("2147483648",&t)==YT_OVERFLOW);
    CHECK(tableau_parse("-2147483648",&t)==YT_OK);
    CHECK(tableau_parse(" [ ] ",&t)==YT_OK && t.shape.count==0);
}
static bool equal_tableau(const Tableau *a,const Tableau *b)
{
    if(!equal_partition(a->shape,b->shape)) return false;
    for(int r=0;r<a->shape.count;++r) for(int c=0;c<a->shape.rows[r];++c) if(a->entries[r][c]!=b->entries[r][c]) return false;
    return true;
}

static void tableau_operations(void)
{
    Tableau input,result,want,restored;

    CHECK(tableau_parse("1,1,3;2,3",&input)==YT_OK);
    CHECK(tableau_semistandard(&input));
    CHECK(tableau_standardize(&input,&result)==YT_OK);
    CHECK(tableau_parse("1,2,5;3,4",&want)==YT_OK);
    CHECK(equal_tableau(&result,&want));
    CHECK(tableau_validate(&result.shape,&result,false).standard);

    CHECK(tableau_parse("1,2,4;3,5;6",&input)==YT_OK);
    Cell added={0};
    CHECK(tableau_row_insert(&input,2,&result,&added)==YT_OK);
    CHECK(added.row==4 && added.column==1);
    CHECK(tableau_parse("1,2,2;3,4;5;6",&want)==YT_OK);
    CHECK(equal_tableau(&result,&want));
    CHECK(tableau_semistandard(&result));

    int ejected=0;
    CHECK(tableau_reverse_insert(&result,added,&restored,&ejected)==YT_OK);
    CHECK(ejected==2);
    CHECK(equal_tableau(&restored,&input));
    CHECK(tableau_reverse_insert(&result,(Cell){1,1},&restored,&ejected)==YT_MALFORMED);

    CHECK(tableau_standardize(&(Tableau){.shape={2,{2,2}},.entries={{2,1},{3,4}}},&result)==YT_MALFORMED);

    CHECK(tableau_parse("1,2,5;3,4",&input)==YT_OK);
    CHECK(tableau_transpose(&input,&result)==YT_OK);
    CHECK(tableau_parse("1,3;2,4;5",&want)==YT_OK);
    CHECK(equal_tableau(&result,&want));

    Partition p=partition("2,1"),changed;
    CHECK(partition_add_cell(&p,(Cell){2,2},&changed)==YT_OK);
    CHECK(equal_partition(changed,partition("2,2")));
    CHECK(partition_add_cell(&p,(Cell){2,3},&changed)==YT_MALFORMED);
    p=partition("2,2");
    CHECK(partition_remove_cell(&p,(Cell){2,2},&changed)==YT_OK);
    CHECK(equal_partition(changed,partition("2,1")));
    CHECK(partition_remove_cell(&p,(Cell){1,2},&changed)==YT_MALFORMED);
}
static void rsk(void)
{
    struct { const char *input,*p,*q; } known[]={
        {"[]","[]","[]"},{"1","1","1"},{"1,2,3","1,2,3","1,2,3"},
        {"3,2,1","1;2;3","1;2;3"},{"2,1,3","1,3;2","1,3;2"},
        {"3,1,4,2","1,2;3,4","1,3;2,4"},
        {"2,4,1,3","1,3;2,4","1,2;3,4"},
        {"4,1,3,2","1,2;3;4","1,3;2;4"}
    };
    for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);++i) {
        Tableau p,q,want_p,want_q; CHECK(permutation_rsk(known[i].input,&p,&q)==YT_OK);
        CHECK(tableau_parse(known[i].p,&want_p)==YT_OK); CHECK(tableau_parse(known[i].q,&want_q)==YT_OK);
        CHECK(equal_tableau(&p,&want_p)); CHECK(equal_tableau(&q,&want_q));
        CHECK(tableau_validate(&p.shape,&p,false).standard); CHECK(tableau_validate(&p.shape,&q,false).standard);
    }
    Tableau p,q; const char *bad[]={"1,1","0,1","1,3","-1","2,1x","1,","1;2"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i) CHECK(permutation_rsk(bad[i],&p,&q)==YT_MALFORMED);
    int values[]={1,2,3,4,5,6}; bool more=true;
    while(more) {
        char text[64]; snprintf(text,sizeof(text),"%d,%d,%d,%d,%d,%d",values[0],values[1],values[2],values[3],values[4],values[5]);
        CHECK(permutation_rsk(text,&p,&q)==YT_OK);
        CHECK(tableau_validate(&p.shape,&p,false).standard); CHECK(tableau_validate(&p.shape,&q,false).standard);
        int best=0,length[6]={0}; for(int i=0;i<6;++i) { length[i]=1; for(int j=0;j<i;++j) if(values[j]<values[i] && length[j]+1>length[i]) length[i]=length[j]+1; if(length[i]>best) best=length[i]; }
        CHECK(p.shape.rows[0]==best);
        int pivot=4; while(pivot>=0 && values[pivot]>values[pivot+1]) --pivot;
        if(pivot<0) more=false;
        else { int j=5; while(values[j]<values[pivot]) --j; int swap=values[j]; values[j]=values[pivot]; values[pivot]=swap; for(int a=pivot+1,b=5;a<b;++a,--b) { swap=values[a]; values[a]=values[b]; values[b]=swap; } }
    }
}
static void rsk_words_and_trace(void)
{
    Tableau p,q,want_p,want_q;
    CHECK(word_rsk("2,1,2,1",&p,&q)==YT_OK);
    CHECK(tableau_parse("1,1;2,2",&want_p)==YT_OK);
    CHECK(tableau_parse("1,3;2,4",&want_q)==YT_OK);
    CHECK(equal_tableau(&p,&want_p));
    CHECK(equal_tableau(&q,&want_q));
    CHECK(word_rsk("0,1",&p,&q)==YT_MALFORMED);
    CHECK(word_rsk("-1",&p,&q)==YT_MALFORMED);

    RSKTrace trace;
    CHECK(permutation_rsk_trace("3,1,4,2",2,&trace)==YT_OK);
    CHECK(trace.count==4 && trace.step==2 && trace.inserted==1);
    CHECK(trace.path_count==2);
    CHECK(trace.path[0].row==1 && trace.path[0].column==1);
    CHECK(trace.path[1].row==2 && trace.path[1].column==1);
    CHECK(tableau_parse("1;3",&want_p)==YT_OK);
    CHECK(equal_tableau(&trace.p,&want_p));
    CHECK(permutation_rsk_trace("3,1,4,2",99,&trace)==YT_OK);
    CHECK(trace.complete && trace.step==4);
    CHECK(permutation_rsk("3,1,4,2",&p,&q)==YT_OK);
    CHECK(equal_tableau(&trace.p,&p) && equal_tableau(&trace.q,&q));
}

static void jeu_de_taquin(void)
{
    JeuState tableau;
    CHECK(jeu_state_parse("3,2,1","1","1,3;2,5;4",&tableau)==YT_OK);
    SkewValidation validation=skew_tableau_validate(&tableau.filling);
    CHECK(validation.standard && validation.semistandard);
    CHECK(jeu_can_begin(&tableau,(Cell){1,1}));
    CHECK(jeu_begin(&tableau,(Cell){1,1})==YT_OK);
    CHECK(tableau.active && tableau.hole.row==1 && tableau.hole.column==1);
    CHECK(jeu_step(&tableau)==JEU_MOVED);
    CHECK(tableau.filling.entries[0][0]==1 && tableau.hole.column==2);
    CHECK(jeu_step(&tableau)==JEU_MOVED);
    CHECK(tableau.filling.entries[0][1]==3 && tableau.hole.column==3);
    CHECK(jeu_step(&tableau)==JEU_FINISHED);
    CHECK(!tableau.active && tableau.filling.shape.inner.count==0);
    CHECK(equal_partition(tableau.filling.shape.outer,partition("2,2,1")));
    CHECK(skew_tableau_validate(&tableau.filling).standard);

    CHECK(jeu_state_parse("3,2,1","1","1,3;2,5;4",&tableau)==YT_OK);
    CHECK(jeu_rectify(&tableau)==YT_OK);
    CHECK(equal_partition(tableau.filling.shape.outer,partition("2,2,1")));
    CHECK(tableau.filling.shape.inner.count==0 && skew_tableau_validate(&tableau.filling).standard);

    CHECK(jeu_state_parse("2,2","1","1;1,2",&tableau)==YT_OK);
    validation=skew_tableau_validate(&tableau.filling);
    CHECK(validation.semistandard && !validation.standard);
    CHECK(jeu_begin(&tableau,(Cell){1,1})==YT_OK);
    CHECK(jeu_step(&tableau)==JEU_MOVED);
    CHECK(tableau.hole.row==2 && tableau.hole.column==1);
    while(tableau.active) CHECK(jeu_step(&tableau)!=JEU_INVALID);
    CHECK(skew_tableau_validate(&tableau.filling).semistandard);

    CHECK(jeu_state_parse("2,1","2","1;2",&tableau)==YT_MALFORMED);
    CHECK(jeu_state_parse("2,1","1","1;2",&tableau)==YT_OK);
    CHECK(jeu_begin(&tableau,(Cell){1,2})==YT_MALFORMED);
}

static void interaction(void)
{
    Controls *u=calloc(1,sizeof(*u)); Console *c=calloc(1,sizeof(*c)); CHECK(u && c);
    controls_init(u); controls_begin(u,576,600,false);
    controls_add(u,42,BUTTON,"Action",60,NULL); controls_add(u,7,FIELD,"Input",60,NULL);
    for(int i=0;i<20;++i) controls_add(u,0,LABEL,"label",60,NULL);
    controls_end(u);
    Rect r=u->controls[0].rect;
    CHECK(controls_hit(u,r.x,r.y)==42); CHECK(controls_hit(u,r.x+r.w,r.y)==0); CHECK(controls_hit(u,-1,r.y)==0);
    controls_touch(u,TOUCH_DOWN,1,r.x+1,r.y+1); ControlEvent e=controls_touch(u,TOUCH_UP,1,r.x+1,r.y+1); CHECK(e.kind==EVENT_ACTIVATE && e.id==42);
    controls_touch(u,TOUCH_DOWN,1,r.x+1,r.y+1); controls_touch(u,TOUCH_MOVE,1,r.x+1,r.y+200); e=controls_touch(u,TOUCH_UP,1,r.x+1,r.y+1); CHECK(e.kind==EVENT_NONE);
    controls_touch(u,TOUCH_DOWN,1,r.x+1,r.y+1); controls_touch(u,TOUCH_CANCEL,1,0,0); e=controls_touch(u,TOUCH_UP,1,r.x+1,r.y+1); CHECK(e.kind==EVENT_NONE);
    controls_touch(u,TOUCH_DOWN,1,r.x+1,r.y+1); controls_touch(u,TOUCH_DOWN,2,r.x+1,r.y+1); e=controls_touch(u,TOUCH_UP,2,r.x+1,r.y+1); CHECK(e.kind==EVENT_NONE && u->pointer==1); controls_touch(u,TOUCH_CANCEL,1,0,0);
    controls_scroll(u,999999); CHECK(u->scroll==u->content-u->height); controls_scroll(u,-99); CHECK(u->scroll==0);
    r=u->controls[1].rect; controls_scroll(u,r.y); CHECK(controls_hit(u,r.x+1,1)==7);
    controls_touch(u,TOUCH_DOWN,3,r.x+1,1); e=controls_touch(u,TOUCH_UP,3,r.x+1,1); CHECK(e.kind==EVENT_FOCUS && u->focus==7);
    console_init(c);
    console_layout(c,u,576,1152);
    bool saw_size=false,saw_dimension=false,saw_hero=false;
    for(int i=0;i<u->count;++i) {
        saw_size=saw_size || strstr(u->controls[i].text,"|λ| = 6");
        saw_dimension=saw_dimension || strstr(u->controls[i].text,"standard tableaux = 16");
        saw_hero=saw_hero || u->controls[i].kind==HERO;
    }
    CHECK(saw_size && saw_dimension && !saw_hero);
    CHECK(raster_text_width("λ",2)==raster_text_width("x",2));
    CHECK(raster_text_width("μ",2)==raster_text_width("m",2));
    CHECK(raster_text_width("ν",2)==raster_text_width("v",2));
    CHECK(raster_text_width("ℕ",2)==raster_text_width("N",2));
    CHECK(raster_text_width("−",2)==raster_text_width("-",2));
    CHECK(raster_text_width("÷",2)==raster_text_width("/",2));
    CHECK(raster_text_width("×",2)==raster_text_width("x",2));
    CHECK(raster_text_width("²",3)==raster_text_width("^2",3));
    CHECK(raster_text_width("₁",3)==raster_text_width("_1",3));
    CHECK(c->wegert.valid); CHECK(c->wegert.n_lambda==4); CHECK(c->wegert.max_hook==5);
    CHECK(c->jeu_loaded && c->jeu_ok);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,JDT_STEP});
    CHECK(c->jeu.active && c->jeu.hole.row==1 && c->jeu.hole.column==2);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,JDT_SLIDE});
    CHECK(!c->jeu.active && c->jeu.filling.shape.inner.count==0);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,JDT_RESET});
    CHECK(c->jeu_ok && c->jeu.filling.shape.inner.count==1);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,JDT_RECTIFY});
    CHECK(c->jeu_ok && c->jeu.filling.shape.inner.count==0);
    CHECK(c->wegert.hook_counts[1]==3 && c->wegert.hook_counts[3]==2 && c->wegert.hook_counts[5]==1);
    u->focus=SET_LAMBDA; console_key(c,u,15); console_key(c,u,1); console_key(c,u,10); console_key(c,u,0);
    CHECK(strcmp(c->fields[SET_LAMBDA],"2,1")==0); CHECK(strstr(c->output[2],"Hook-length formula gives 2 standard tableaux."));
    console_key(c,u,14); CHECK(!c->partition_ok); console_key(c,u,0); CHECK(c->partition_ok); console_key(c,u,19); CHECK(!u->focus);
    console_run(c,LittlewoodRichardsonCoefficient); CHECK(strstr(c->output[5],"NOT IMPLEMENTED")); CHECK(strstr(c->output[5],"(Partition, Partition, Partition)")); CHECK(strstr(c->output[5],"output: Nat"));
    console_run(c,RSKMatrix); CHECK(c->rsk_ok && c->rsk_total==2);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,RSK_START});
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,RSK_NEXT});
    CHECK(c->rsk_input_kind==RSK_MATRIX_INPUT && c->p.values[0][0]==1);
    console_run(c,RSKBiword); CHECK(c->rsk_ok && c->rsk_total==3 && c->q.values[0][1]==1);
    console_run(c,RSKWord); CHECK(c->rsk_ok && c->rsk_input_kind==RSK_WORD_INPUT);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,RSK_START}); CHECK(c->rsk_step==0);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,RSK_NEXT}); CHECK(c->rsk_step==1 && c->p.count==1);
    console_run(c,RSKPermutation); CHECK(c->rsk_ok && c->rsk_input_kind==RSK_PERMUTATION_INPUT && c->rsk_step==c->rsk_total);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,STEP_PLUS}); CHECK(strcmp(c->fields[SET_N],"7")==0);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,CHOOSE_STANDARD}); CHECK(c->decreasing);
    console_event(c,u,(ControlEvent){EVENT_ACTIVATE,CHOOSE_TABLEAU_KIND});
    CHECK(c->tableau_ok && strstr(c->output[1],"ArbitraryFilling: YES"));
    TileProjection before_column=c->tableau;
    c->insertion=COLUMN_INSERTION;
    console_run(c,InsertLetter);
    CHECK(strstr(c->output[1],"NOT IMPLEMENTED: ColumnInsertion"));
    CHECK(memcmp(&before_column,&c->tableau,sizeof(before_column))==0);
    console_run(c,ReverseInsert);
    CHECK(strstr(c->output[1],"NOT IMPLEMENTED: ColumnInsertion"));
    c->insertion=ROW_INSERTION;
    console_layout(c,u,576,1152); CHECK(u->content>u->height); int separators=0,wegert=0;
    bool ids[OP_BASE+OP_COUNT]={false};
    for(int i=0;i<u->count;++i) {
        if(u->controls[i].kind==SEPARATOR) ++separators;
        if(u->controls[i].kind==WEGERT) ++wegert;
        int id=u->controls[i].id; if(id) { CHECK(!ids[id]); ids[id]=true; }
    }
    CHECK(separators==6); CHECK(wegert==1);
    for(int op=0;op<OP_COUNT;++op) {
        bool visible=op==RSKPermutation || op==RSKWord
                  || op==JeuDeTaquinSlide || op==Rectify;
        CHECK(ids[OP_BASE+op]==visible);
    }
    console_run(c,ConjugatePartition); CHECK(strstr(c->output[0],"conjugate = "));
    console_run(c,ListCells); CHECK(strstr(c->output[0],"cells: "));

    snprintf(c->fields[SET_TABLEAU],512,"1,1,3;2,3");
    console_run(c,StandardizeTableau); CHECK(c->tableau_ok && strstr(c->output[1],"standardized"));
    snprintf(c->fields[SET_TABLEAU],512,"1,2,4;3,5;6");
    snprintf(c->fields[SET_ENTRY],512,"2");
    console_run(c,InsertLetter); CHECK(c->tableau_ok && strstr(c->output[1],"new cell"));
    snprintf(c->fields[SET_CELL],512,"3,1");
    console_run(c,ReverseInsert); CHECK(c->tableau_ok && strstr(c->output[1],"ejected"));
    console_run(c,TransposeTableau); CHECK(c->tableau_ok && strstr(c->output[1],"transpose"));
    snprintf(c->fields[SET_LAMBDA],512,"3,2,1");
    console_run(c,DisplayPartition);
    snprintf(c->fields[SET_CELL],512,"2,3");
    console_run(c,AddCell); CHECK(strstr(c->output[2],"add (2,3)"));
    snprintf(c->fields[SET_CELL],512,"2,2");
    console_run(c,RemoveCell); CHECK(strstr(c->output[2],"remove (2,2)"));
    u->focus=SET_LAMBDA; console_layout(c,u,576,1152); CHECK(u->height==882);
    CHECK(console_key_hit(u,0,897,1152)==0); CHECK(console_key_hit(u,575,1136,1152)==19); CHECK(console_key_hit(u,576,897,1152)==-1);
    free(u); free(c);
}
#include "nearby_tests.inc"
static void rsk_result_families(void)
{
    RSKTrace trace;
    Permutation permutation={3,{3,1,2}};
    PermutationRSKResult standard_pair={0},saved_pair={0};
    WordRSKResult word_pair;
    BiwordRSKResult biword_pair;
    CHECK(rsk_permutation(&permutation,1,&trace)==YT_OK);
    CHECK(rsk_permutation_result(&trace,&standard_pair)==YT_MALFORMED);
    CHECK(rsk_permutation(&permutation,3,&trace)==YT_OK);
    CHECK(rsk_permutation_result(&trace,&standard_pair)==YT_OK);
    CHECK(tableau_validate(&standard_pair.insertion.filling.shape,&standard_pair.insertion.filling,false).standard);
    CHECK(tableau_validate(&standard_pair.recording.filling.shape,&standard_pair.recording.filling,false).standard);
    saved_pair=standard_pair;
    Word word={4,{2,1,2,1}};
    CHECK(rsk_word(&word,4,&trace)==YT_OK);
    CHECK(rsk_word_result(&trace,&word_pair)==YT_OK);
    CHECK(word_pair.insertion.filling.entries[0][1]==1);
    CHECK(rsk_permutation_result(&trace,&standard_pair)==YT_MALFORMED);
    CHECK(memcmp(&standard_pair,&saved_pair,sizeof(standard_pair))==0);
    Biword biword;
    CHECK(biword_parse("1,1,2;1,2,1",&biword)==YT_OK);
    CHECK(rsk_biword(&biword,3,&trace)==YT_OK);
    CHECK(rsk_biword_result(&trace,&biword_pair)==YT_OK);
    CHECK(biword_pair.recording.filling.entries[0][1]==1);
    CHECK(rsk_word_result(&trace,&word_pair)==YT_MALFORMED);
    CHECK(rsk_permutation(&permutation,3,&trace)==YT_OK);
    trace.q=(Tableau){.shape={1,{3}},.entries={{1,2,3}}};
    StandardTableau other_shape;
    CHECK(tableau_as_standard(&trace.q,&other_shape)==YT_OK);
    CHECK(rsk_permutation_result(&trace,&standard_pair)==YT_MALFORMED);
    CHECK(memcmp(&standard_pair,&saved_pair,sizeof(standard_pair))==0);
    trace.p.shape.count=YT_DIM+1;
    CHECK(rsk_permutation_result(&trace,&standard_pair)==YT_MALFORMED);
    CHECK(rsk_word_result(NULL,&word_pair)==YT_MALFORMED);
    CHECK(rsk_biword_result(&trace,NULL)==YT_MALFORMED);
}
int main(void) { partitions(); tableaux(); tableau_operations(); rsk(); rsk_words_and_trace(); rsk_result_families(); jeu_de_taquin(); interaction(); nearby_tests(); printf("PASS %u checks: mathematics, RSK, jeu de taquin, controls, scrolling, console\n",checks); return 0; }
