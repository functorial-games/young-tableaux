#include "console.h"
#include "nearby.h"
#include "parse.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <limits.h>
const OperationInfo operation_info[OP_COUNT]={
#define OP(symbol,section,label,input,output) [symbol]={label,input,output,section},
#include "operations.def"
#undef OP
};
static const char *sections[]={"Partition / Young diagram","Tableau","Hooks / corners / cells","RSK","Jeu de taquin","Littlewood-Richardson","Symmetric group / representations","Symmetric functions","Young graph / branching","Random / asymptotic","Type-A / Coxeter","Global conventions"};
static const char *tableau_kinds[]={"Standard","ArbitraryFilling","RowStandard","ColumnStandard","Semistandard"};
static const char *field_names[FIELD_COUNT]={"","λ: rows","tableau: rows separated by ;","permutation","μ: partition","ν: partition","selected cell: row,column","word","biword: top ; bottom","matrix: rows separated by ;","n","alphabet maximum","weight / content","characteristic: 0 or p","prime p","Hecke parameter","basis","q","t","variables / specialization","coefficients","Young graph path","probability law","simple transposition word","second permutation","reading convention","left / right action","skew tableau: visible rows after μ","entry"};
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
static int shape_row(const Partition *p,int row)
{ return row>=0 && row<p->count?p->rows[row]:0; }
static void skew_tiles(TileProjection *out,const JeuState *t)
{
    memset(out,0,sizeof(*out)); out->count=t->filling.shape.outer.count; out->numbers=true;
    for(int row=0;row<t->filling.shape.outer.count;++row) {
        int start=shape_row(&t->filling.shape.inner,row);
        out->starts[row]=start; out->rows[row]=t->filling.shape.outer.rows[row]-start;
        for(int local=0;local<out->rows[row];++local) {
            int column=start+local;
            out->values[row][local]=t->filling.entries[row][column];
            if(t->active && row+1==t->hole.row && column+1==t->hole.column)
                out->marks[row][local]=2;
            else if(t->active && ((row+1==t->hole.row && column==t->hole.column)
                    || (row==t->hole.row && column+1==t->hole.column)))
                out->marks[row][local]=1;
        }
    }
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
    if(s!=YT_OK) { append(c->output[0],"%s\n",math_status(s)); append(c->output[2],"λ: %s\n",math_status(s)); return; }
    Partition conjugate=partition_conjugate(&p);
    tiles(&c->partition,&p,NULL); tiles(&c->conjugate,&conjugate,NULL);
    append(c->output[0],"Valid partition\nsize |λ| = %d\nconjugate = ",partition_size(&p)); partition_text(c->output[0],&conjugate);
    Cell cells[YT_CELLS+1]; int n=partition_cells(&p,cells); cells_text(c->output[0],"cells",cells,n);
    n=partition_removable(&p,cells);
    n=partition_addable(&p,cells);
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
    tiles(&c->hooks,&p,&h);
    uint64_t value; s=partition_hook_product(&p,&value);
    if(s==YT_OK) append(c->output[2],"Hook product = %" PRIu64 "\n",value); else append(c->output[2],"Hook product: %s\n",math_status(s));
    s=partition_standard_count(&p,&value);
    if(s==YT_OK) append(c->output[2],"Hook-length formula gives %" PRIu64 " standard tableaux.\n",value);
    else append(c->output[2],"Standard tableaux: %s\n",math_status(s));
}
static void refresh_tableau(Console *c)
{
    Partition p; Tableau t; MathStatus a=partition_parse(c->fields[SET_LAMBDA],&p), b=tableau_parse(c->fields[SET_TABLEAU],&t);
    c->output[1][0]=0; c->tableau_ok=false;
    if(c->tableau_kind>=TABLEAU_SKEW) {
        append(c->output[1],"UNSUPPORTED TABLEAU KIND: choose an ordinary filling kind.");
        return;
    }
    if(a!=YT_OK || b!=YT_OK) { append(c->output[1],"%s: %s\n",a!=YT_OK?"λ":"tableau",math_status(a!=YT_OK?a:b)); return; }
    bool kind_valid=false;
    MathStatus kind_status=tableau_check_kind(&t,c->tableau_kind,c->decreasing,&kind_valid);
    if(kind_status!=YT_OK) { append(c->output[1],"kind: %s",math_status(kind_status)); return; }
    append(c->output[1],"%s: %s\n",tableau_kinds[c->tableau_kind],kind_valid?"YES":"NO");
    Validation v=tableau_validate(&p,&t,c->decreasing);
    append(c->output[1],"shape compatible: %s\nrows strictly %s: %s\ncolumns strictly %s: %s\nstandard (1..n once): %s\n",v.shape?"YES":"NO",c->decreasing?"decreasing":"increasing",v.rows?"YES":"NO",c->decreasing?"decreasing":"increasing",v.columns?"YES":"NO",v.standard?"YES":"NO");
    tableau_text(c->output[1],"filling",&t); tiles(&c->tableau,&t.shape,&t); c->tableau_ok=true;
}
static MathStatus trace_rsk_input(const Console *c,RSKTrace *trace)
{
    switch(c->rsk_input_kind) {
    case RSK_PERMUTATION_INPUT: return permutation_rsk_trace(c->fields[SET_PERMUTATION],c->rsk_step,trace);
    case RSK_WORD_INPUT: return word_rsk_trace(c->fields[SET_WORD],c->rsk_step,trace);
    case RSK_BIWORD_INPUT: {
        Biword input;
        MathStatus status=biword_parse(c->fields[SET_BIWORD],&input);
        return status==YT_OK?rsk_biword(&input,c->rsk_step,trace):status;
    }
    case RSK_MATRIX_INPUT: {
        NatMatrix input;
        MathStatus status=nat_matrix_parse(c->fields[SET_MATRIX],&input);
        return status==YT_OK?rsk_matrix(&input,c->rsk_step,trace):status;
    }
    }
    return YT_MALFORMED;
}

static MathStatus check_completed_rsk_result(const Console *c,const RSKTrace *trace)
{
    if(!trace->complete) return YT_OK;
    switch(c->rsk_input_kind) {
    case RSK_PERMUTATION_INPUT: {
        PermutationRSKResult result;
        return rsk_permutation_result(trace,&result);
    }
    case RSK_WORD_INPUT: {
        WordRSKResult result;
        return rsk_word_result(trace,&result);
    }
    case RSK_BIWORD_INPUT: case RSK_MATRIX_INPUT: {
        BiwordRSKResult result;
        return rsk_biword_result(trace,&result);
    }
    }
    return YT_MALFORMED;
}

static void refresh_rsk(Console *c)
{
    c->output[3][0]=0; c->rsk_ok=false;
    if(c->insertion) { append(c->output[3],"UNSUPPORTED CONVENTION: this contract uses row insertion."); return; }
    RSKTrace trace;
    MathStatus status=trace_rsk_input(c,&trace);
    if(status==YT_OK) status=check_completed_rsk_result(c,&trace);
    if(status!=YT_OK) { append(c->output[3],"RSK input: %s",math_status(status)); return; }
    c->rsk_step=trace.step; c->rsk_total=trace.count;
    const char *names[]={"PERMUTATION","WORD","BIWORD","MATRIX"};
    append(c->output[3],"%s row RSK\nstep %d / %d\n",names[c->rsk_input_kind],trace.step,trace.count);
    if(trace.step) {
        append(c->output[3],"insert %d\nbump path: ",trace.inserted);
        for(int i=0;i<trace.path_count;++i) append(c->output[3],"%s(%d,%d)",i?" -> ":"",trace.path[i].row,trace.path[i].column);
        append(c->output[3],"\n");
    } else append(c->output[3],"Press NEXT to insert the first entry.\n");
    tableau_text(c->output[3],"P: insertion tableau",&trace.p);
    tableau_text(c->output[3],"Q: recording tableau",&trace.q);
    tiles(&c->p,&trace.p.shape,&trace.p); tiles(&c->q,&trace.q.shape,&trace.q);
    for(int i=0;i<trace.path_count;++i) {
        int row=trace.path[i].row-1,column=trace.path[i].column-1;
        if(row>=0 && row<c->p.count && column>=0 && column<c->p.rows[row]) c->p.marks[row][column]=1;
    }
    c->rsk_ok=true;
}
static MathStatus selected_cell(const char *text,Cell *cell)
{
    int row,column; char extra;
    if(sscanf(text," %d , %d %c",&row,&column,&extra)!=2 || row<1 || column<1) return YT_MALFORMED;
    *cell=(Cell){row,column}; return YT_OK;
}

static MathStatus integer_field(const char *text,int *value)
{
    if(!text || !value) return YT_MALFORMED;
    const char *cursor=text; int values[YT_DIM],count=0;
    MathStatus status=parse_integer_list(&cursor,values,&count,false);
    if(status!=YT_OK) return status;
    parse_spaces(&cursor);
    if(*cursor || count!=1) return YT_MALFORMED;
    *value=values[0];
    return YT_OK;
}
static void project_jeu(Console *c)
{
    c->output[4][0]=0; c->jeu_loaded=true; skew_tiles(&c->jeu_tiles,&c->jeu);
    append(c->output[4],"Forward jeu de taquin\nouter λ = "); partition_text(c->output[4],&c->jeu.filling.shape.outer);
    append(c->output[4],"inner μ = "); partition_text(c->output[4],&c->jeu.filling.shape.inner);
    if(c->jeu.active) {
        c->jeu_ok=true;
        append(c->output[4],"active hole = (%d,%d)\nSTEP moves the smaller right/below entry; ties move the lower entry.\n",c->jeu.hole.row,c->jeu.hole.column);
    } else {
        SkewValidation v=skew_tableau_validate(&c->jeu.filling);
        c->jeu_ok=v.semistandard;
        append(c->output[4],"rows weakly increasing: %s\ncolumns strictly increasing: %s\npositive entries: %s\nsemistandard: %s\nstandard: %s\n",
               v.rows_weak?"YES":"NO",v.columns_strict?"YES":"NO",v.positive?"YES":"NO",v.semistandard?"YES":"NO",v.standard?"YES":"NO");
    }
}
static void refresh_jeu(Console *c)
{
    memset(&c->jeu,0,sizeof(c->jeu)); memset(&c->jeu_tiles,0,sizeof(c->jeu_tiles));
    c->jeu_loaded=false; c->jeu_ok=false; c->output[4][0]=0;
    MathStatus status=jeu_state_parse(c->fields[SET_LAMBDA],c->fields[SET_MU],c->fields[SET_SKEW_TABLEAU],&c->jeu);
    if(status!=YT_OK) {
        append(c->output[4],"skew tableau: %s\nUse λ as outer shape, μ as inner shape, and enter only the visible skew cells in each row.",math_status(status));
        return;
    }
    project_jeu(c);
}
static bool finish_jeu_slide(Console *c)
{
    while(c->jeu.active) if(jeu_step(&c->jeu)==JEU_INVALID) {
        c->jeu_ok=false; snprintf(c->output[4],UI_TEXT,"INVALID jeu de taquin state"); return false;
    }
    project_jeu(c); return c->jeu_ok;
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
    layout_add(c,SCRIPT_LABEL,0,"Young Tableaux 0.4.0");
    layout_add(c,SCRIPT_SEPARATOR,0,"");
    layout_add(c,SCRIPT_FIELD,SET_LAMBDA,"λ: rows");
    layout_add(c,SCRIPT_SHAPE,0,"");
    layout_add(c,SCRIPT_WEGERT,160,"");
    layout_add(c,SCRIPT_PLOT_CONTROLS,0,"");
    layout_add(c,SCRIPT_SEPARATOR,0,"");
    layout_add(c,SCRIPT_HOOKS,0,"Hook lengths: each number counts its cell, cells right, and cells below.");
    layout_add(c,SCRIPT_OUTPUT,2,"");
    layout_add(c,SCRIPT_FACTS,0,"");
    layout_add(c,SCRIPT_SEPARATOR,0,"");
    layout_add(c,SCRIPT_LABEL,0,"More operations");
}
void console_init(Console *c)
{
    memset(c,0,sizeof(*c));
    const char *defaults[FIELD_COUNT]={"","3,2,1","1,2,4;3,5;6","3,1,4,2","1","3,2,1","1,1","1,2,1","1,1,2;1,2,1","1,0;0,1","6","3","2,1","0","2","1","Schur","1","1","x1,x2","1","[]","Plancherel","1,2,1","1,2,3,4","RowReading","LeftAction","1,3;2,5;4","7"};
    for(int i=1;i<FIELD_COUNT;++i) snprintf(c->fields[i],sizeof(c->fields[i]),"%s",defaults[i]);
    for(int i=4;i<11;++i) snprintf(c->output[i],UI_TEXT,"Select an operation to inspect its input/output types.");
    snprintf(c->output[11],UI_TEXT,"English: top row longest.\nCells use one-based (row,column).\nContent default: column−row.\nRow RSK bumps the first strictly greater entry.\nJeu de taquin uses weak rows / strict columns; ties between right and below move the lower entry.\nSkew rows contain only visible cells after μ.");
    snprintf(c->scripted_facts,UI_TEXT,"Schur specialization\nSubstitute 1, z, z², … into s_λ to get one function of z.");
    snprintf(c->fields[SET_RECORDING],512,"1,3;2,4");
    snprintf(c->fields[SET_SEED],512,"1");
    snprintf(c->fields[SET_INNER_FUNCTION],512,"0;1;1");
    snprintf(c->fields[SET_COEFFICIENTS],512,"0;1;2");
    snprintf(c->fields[SET_BASIS],512,"1");
    snprintf(c->fields[SET_VARIABLES],512,"1,2");
    c->rng.state=1;
    default_layout(c); c->rsk_step=YT_DIM;
    refresh_partition(c); refresh_tableau(c); refresh_rsk(c); refresh_jeu(c);
}
static void number_list_text(char *out,const int *values,int count)
{
    append(out,"[");
    for(int index=0;index<count;++index) append(out,"%s%d",index?",":"",values[index]);
    append(out,"]");
}
static bool advanced_run(Console *c,Operation operation)
{
    char *output=c->output[operation_info[operation].section];
    MathStatus status=YT_OK;
    Partition first={0},second={0},third={0};
    Permutation permutation={0};
    StandardTableau standard,result;
    Tableau filling;
    NumberList list={0};
    SymmetricFunction function,other,answer;
    int number=0;
    uint64_t coefficient=0;
    int64_t character=0;
    switch(operation) {
    case InverseRSK: {
        PermutationRSKResult input;
        status=tableau_parse(c->fields[SET_TABLEAU],&filling);
        if(status==YT_OK) status=tableau_as_standard(&filling,&input.insertion);
        if(status==YT_OK) status=tableau_parse(c->fields[SET_RECORDING],&filling);
        if(status==YT_OK) status=tableau_as_standard(&filling,&input.recording);
        if(status==YT_OK) status=inverse_rsk(&input,&permutation);
        output[0]=0;
        if(status==YT_OK) number_list_text(output,permutation.values,permutation.count);
        break;
    }
    case Promote: case Evacuate:
        status=tableau_parse(c->fields[SET_TABLEAU],&filling);
        if(status==YT_OK) status=tableau_as_standard(&filling,&standard);
        if(status==YT_OK) status=operation==Promote?tableau_promote(&standard,&result):tableau_evacuate(&standard,&result);
        output[0]=0;
        if(status==YT_OK) tableau_text(output,operation==Promote?"Promotion":"Evacuation",&result.filling);
        break;
    case LittlewoodRichardsonCoefficient: case EnumerateLRTableaux: case MultiplySchurFunctions:
        status=partition_parse(c->fields[SET_LAMBDA],&first);
        if(status==YT_OK) status=partition_parse(c->fields[SET_MU],&second);
        output[0]=0;
        if(operation==MultiplySchurFunctions) {
            if(status==YT_OK) status=symmetric_schur_product(&first,&second,&answer);
            if(status==YT_OK) status=symmetric_text(&answer,output,UI_TEXT);
        } else {
            if(status==YT_OK) status=partition_parse(c->fields[SET_NU],&third);
            if(operation==LittlewoodRichardsonCoefficient) {
                if(status==YT_OK) status=littlewood_richardson_coefficient(&first,&second,&third,&coefficient);
                if(status==YT_OK) append(output,"c(λ, μ; ν) = %" PRIu64,coefficient);
            } else {
                /* The large result lives on the heap on Android too. */
                SkewTableauList *tableaux=malloc(sizeof(*tableaux));
                if(!tableaux) status=YT_LIMIT;
                if(status==YT_OK) status=littlewood_richardson_tableaux(&first,&second,&third,tableaux);
                if(status==YT_OK) {
                    append(output,"%d LR tableaux of ν ÷ λ, content μ\n",tableaux->count);
                    for(int index=0;index<tableaux->count;++index) {
                        append(output,"Tableau %d\n",index+1);
                        for(int row=0;row<third.count;++row) {
                            int start=row<first.count?first.rows[row]:0;
                            for(int column=start;column<third.rows[row];++column) append(output,"%d ",tableaux->values[index].entries[row][column]);
                            append(output,"\n");
                        }
                    }
                }
                free(tableaux);
            }
        }
        break;
    case CharacterValue:
        status=partition_parse(c->fields[SET_LAMBDA],&first);
        if(status==YT_OK) status=permutation_parse(c->fields[SET_PERMUTATION],&permutation);
        if(status==YT_OK) status=permutation_cycle_type(&permutation,&second);
        if(status==YT_OK) status=symmetric_character_cycle_type(&first,&second,&character);
        output[0]=0;
        if(status==YT_OK) append(output,"Ordinary complex character χλ = %" PRId64,character);
        break;
    case ChangeBasis: case Specialize: case Plethysm:
        status=symmetric_parse(c->fields[SET_COEFFICIENTS],&function);
        output[0]=0;
        if(operation==ChangeBasis) {
            if(status==YT_OK) status=integer_field(c->fields[SET_BASIS],&number);
            if(status==YT_OK) status=symmetric_change_basis(&function,(SymmetricBasis)number,&answer);
            if(status==YT_OK) status=symmetric_text(&answer,output,UI_TEXT);
        } else if(operation==Plethysm) {
            if(status==YT_OK) status=symmetric_parse(c->fields[SET_INNER_FUNCTION],&other);
            if(status==YT_OK) status=symmetric_plethysm(&function,&other,&answer);
            if(status==YT_OK) status=symmetric_text(&answer,output,UI_TEXT);
        } else {
            const char *cursor=c->fields[SET_VARIABLES]; int alphabet[YT_DIM],count=0; Rational value;
            if(status==YT_OK) status=parse_integer_list(&cursor,alphabet,&count,false);
            parse_spaces(&cursor);
            if(status==YT_OK && *cursor) status=YT_MALFORMED;
            if(status==YT_OK) status=symmetric_specialize(&function,alphabet,count,&value);
            if(status==YT_OK) append(output,"Finite alphabet value = %" PRId64 " ÷ %" PRIu64,value.numerator,value.denominator);
        }
        break;
    case BranchUp: case BranchDown: case EnumerateYoungGraphPaths:
        status=partition_parse(c->fields[SET_LAMBDA],&first);
        output[0]=0;
        if(operation==EnumerateYoungGraphPaths) {
            if(status==YT_OK) status=partition_parse(c->fields[SET_NU],&second);
            PartitionPathList *paths=malloc(sizeof(*paths));
            if(!paths) status=YT_LIMIT;
            if(status==YT_OK) status=young_graph_paths(&first,&second,paths);
            if(status==YT_OK) {
                append(output,"%d paths\n",paths->count);
                for(int path=0;path<paths->count;++path) {
                    for(int step=0;step<paths->values[path].count;++step) {
                        number_list_text(output,paths->values[path].values[step].rows,paths->values[path].values[step].count);
                        append(output,step+1<paths->values[path].count?" → ":"\n");
                    }
                }
            }
            free(paths);
        } else {
            PartitionList partitions;
            if(status==YT_OK) status=operation==BranchUp?partition_branch_up(&first,&partitions):partition_branch_down(&first,&partitions);
            if(status==YT_OK) for(int index=0;index<partitions.count;++index) partition_text(output,&partitions.values[index]);
            if(status==YT_OK && !partitions.count) append(output,"[]");
        }
        break;
    case GenerateRandomPermutation: case GenerateRandomStandardTableau: case SamplePlancherelPartition:
        output[0]=0;
        status=integer_field(c->fields[SET_SEED],&number);
        if(status!=YT_OK || number<0) { status=YT_MALFORMED; break; }
        if(operation==GenerateRandomStandardTableau) {
            status=partition_parse(c->fields[SET_LAMBDA],&first);
            if(status==YT_OK) status=random_standard_tableau(&first,&c->rng,&result);
            if(status==YT_OK) tableau_text(output,"Uniform standard tableau",&result.filling);
        } else {
            status=integer_field(c->fields[SET_N],&number);
            if(operation==GenerateRandomPermutation) {
                if(status==YT_OK) status=random_permutation(number,&c->rng,&permutation);
                if(status==YT_OK) number_list_text(output,permutation.values,permutation.count);
            } else {
                if(status==YT_OK) status=sample_plancherel_partition(number,&c->rng,&first);
                if(status==YT_OK) partition_text(output,&first);
            }
        }
        break;
    case ComputeLongestIncreasingSubsequence: case ComputeLongestDecreasingSubsequence: case CoxeterReducedWord: case BruhatRelations:
        status=permutation_parse(c->fields[SET_PERMUTATION],&permutation);
        output[0]=0;
        if(operation==BruhatRelations) {
            Permutation right; bool relation=false;
            if(status==YT_OK) status=permutation_parse(c->fields[SET_SECOND_PERMUTATION],&right);
            if(status==YT_OK) status=permutation_bruhat_leq(&permutation,&right,&relation);
            if(status==YT_OK) append(output,"Strong Bruhat order: first ≤ second = %s",relation?"YES":"NO");
        } else {
            if(status==YT_OK) status=operation==ComputeLongestIncreasingSubsequence?permutation_lis(&permutation,&list):operation==ComputeLongestDecreasingSubsequence?permutation_lds(&permutation,&list):permutation_coxeter_reduced_word(&permutation,&list);
            if(status==YT_OK) number_list_text(output,list.values,list.count);
        }
        break;
    default: return false;
    }
    if(status==YT_OK && strlen(output)>=UI_TEXT-1) status=YT_LIMIT;
    if(status!=YT_OK) snprintf(output,UI_TEXT,"%s: %s",operation_info[operation].label,math_status(status));
    return true;
}

void console_run(Console *c,Operation op)
{
    if(op<0 || op>=OP_COUNT) return;
    int section=operation_info[op].section;
    if(advanced_run(c,op)) return;
    switch(op) {
    case DisplayPartition: refresh_partition(c); refresh_jeu(c); return;
    case ConjugatePartition: case ListCells: case ComputeHookLengths: case ComputeHookProduct:
    case CountStandardTableaux: case FindCorners: case FindAddableCells: case FindRemovableCells: refresh_partition(c); return;
    case AddCell: case RemoveCell: {
        Partition input,result; Cell cell={0};
        MathStatus status=partition_parse(c->fields[SET_LAMBDA],&input);
        if(status==YT_OK) status=selected_cell(c->fields[SET_CELL],&cell);
        if(status==YT_OK)
            status=op==AddCell?partition_add_cell(&input,cell,&result):partition_remove_cell(&input,cell,&result);
        c->output[2][0]=0;
        if(status!=YT_OK) append(c->output[2],"%s cell: %s",op==AddCell?"add":"remove",math_status(status));
        else {
            append(c->output[2],"%s (%d,%d) -> ",op==AddCell?"add":"remove",cell.row,cell.column);
            partition_text(c->output[2],&result);
        }
        return;
    }
    case DisplayTableau: case ValidateTableau: refresh_tableau(c); return;
    case StandardizeTableau: {
        Tableau input,result;
        MathStatus status=tableau_parse(c->fields[SET_TABLEAU],&input);
        if(status==YT_OK) status=tableau_standardize(&input,&result);
        c->output[1][0]=0; c->tableau_ok=false;
        if(status!=YT_OK) append(c->output[1],"standardize: %s",math_status(status));
        else {
            tableau_text(c->output[1],"standardized",&result);
            tiles(&c->tableau,&result.shape,&result); c->tableau_ok=true;
        }
        return;
    }
    case InsertLetter: {
        if(c->insertion!=ROW_INSERTION) { c->output[1][0]=0; append(c->output[1],"UNSUPPORTED CONVENTION: row insertion only"); return; }
        Tableau input,result; Cell added={0}; int entry=0;
        MathStatus status=tableau_parse(c->fields[SET_TABLEAU],&input);
        if(status==YT_OK) status=integer_field(c->fields[SET_ENTRY],&entry);
        if(status==YT_OK) status=tableau_row_insert(&input,entry,&result,&added);
        c->output[1][0]=0; c->tableau_ok=false;
        if(status!=YT_OK) append(c->output[1],"row insertion: %s",math_status(status));
        else {
            append(c->output[1],"insert %d; new cell (%d,%d)\n",entry,added.row,added.column);
            tableau_text(c->output[1],"result",&result);
            tiles(&c->tableau,&result.shape,&result); c->tableau_ok=true;
        }
        return;
    }
    case ReverseInsert: {
        if(c->insertion!=ROW_INSERTION) { c->output[1][0]=0; append(c->output[1],"UNSUPPORTED CONVENTION: row insertion only"); return; }
        Tableau input,result; Cell corner={0}; int bumped=0;
        MathStatus status=tableau_parse(c->fields[SET_TABLEAU],&input);
        if(status==YT_OK) status=selected_cell(c->fields[SET_CELL],&corner);
        if(status==YT_OK) status=tableau_reverse_insert(&input,corner,&result,&bumped);
        c->output[1][0]=0; c->tableau_ok=false;
        if(status!=YT_OK) append(c->output[1],"reverse insertion: %s",math_status(status));
        else {
            append(c->output[1],"reverse bump from (%d,%d); ejected %d\n",corner.row,corner.column,bumped);
            tableau_text(c->output[1],"result",&result);
            tiles(&c->tableau,&result.shape,&result); c->tableau_ok=true;
        }
        return;
    }
    case TransposeTableau: {
        Tableau input,result;
        MathStatus status=tableau_parse(c->fields[SET_TABLEAU],&input);
        if(status==YT_OK) status=tableau_transpose(&input,&result);
        c->output[1][0]=0; c->tableau_ok=false;
        if(status!=YT_OK) append(c->output[1],"transpose: %s",math_status(status));
        else {
            tableau_text(c->output[1],"transpose",&result);
            tiles(&c->tableau,&result.shape,&result); c->tableau_ok=true;
        }
        return;
    }
    case RSKPermutation: c->rsk_input_kind=RSK_PERMUTATION_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); return;
    case RSKWord: c->rsk_input_kind=RSK_WORD_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); return;
    case RSKBiword: c->rsk_input_kind=RSK_BIWORD_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); return;
    case RSKMatrix: c->rsk_input_kind=RSK_MATRIX_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); return;
    case JeuDeTaquinSlide: {
        if(!c->jeu_ok) { refresh_jeu(c); if(!c->jeu_ok) return; }
        if(!c->jeu.active) {
            Cell cell;
            if(selected_cell(c->fields[SET_CELL],&cell)!=YT_OK || jeu_begin(&c->jeu,cell)!=YT_OK) {
                snprintf(c->output[4],UI_TEXT,"Selected cell must be a removable inner corner of μ."); return;
            }
        }
        finish_jeu_slide(c); return;
    }
    case Rectify: {
        if(!c->jeu_ok) { refresh_jeu(c); if(!c->jeu_ok) return; }
        MathStatus status=jeu_rectify(&c->jeu);
        if(status!=YT_OK) { snprintf(c->output[4],UI_TEXT,"Rectification failed: %s",math_status(status)); c->jeu_ok=false; return; }
        project_jeu(c); return;
    }
    case RepresentationDimension: {
        Partition p; uint64_t count; MathStatus status=partition_parse(c->fields[SET_LAMBDA],&p);
        if(status==YT_OK) status=partition_standard_count(&p,&count);
        c->output[section][0]=0;
        if(status==YT_OK) append(c->output[section],"Ordinary complex Specht dimension = %" PRIu64,count);
        else append(c->output[section],"%s",math_status(status));
        return;
    }
    default: break;
    }
    snprintf(c->output[section],UI_TEXT,"INTERNAL DISPATCH ERROR");
    if(section==1) c->tableau_ok=false;
    if(section==3) c->rsk_ok=false;
}
static void field(Console *c,Controls *u,int id)
{ controls_add(u,0,LABEL,field_names[id],0,NULL); controls_add(u,id,FIELD,c->fields[id],0,NULL); }
static void diagram(Controls *u,const char *label,const TileProjection *p)
{ controls_add(u,0,LABEL,label,0,NULL); controls_add(u,0,DIAGRAM,"",(p->count?p->count:1)*12*u->scale+8*u->scale,p); }
static void button_strip(Controls *u,const int *ids,const char *const *labels,int count)
{
    int scale=u->scale,margin=4*scale,gap=4*scale,width=u->width-2*margin;
    int y=u->content,height=24*scale;
    for(int i=0;i<count;++i) {
        int left=margin+i*(width+gap)/count,right=margin+(i+1)*(width+gap)/count-gap;
        controls_add_at(u,ids[i],BUTTON,labels[i],(Rect){left,y,right-left,height},NULL);
    }
}
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
static bool operation_exposed(Operation op)
{
    return operation_info[op].section>=3;
}
static bool section_exposed(int section)
{
    if(section==11) return true;
    for(int op=0;op<OP_COUNT;++op)
        if(operation_info[op].section==section && operation_exposed((Operation)op)) return true;
    return false;
}
void console_layout(Console *c,Controls *u,int w,int h)
{
    controls_begin(u,w,h,u->focus!=0);
    scripted_top(c,u);

    for(int s=0;s<12;++s) {
        if(!section_exposed(s)) continue;
        controls_add(u,0,SEPARATOR,"",2*u->scale,NULL); controls_add(u,0,LABEL,sections[s],0,NULL);
        switch(s) {
        case 0: controls_add(u,0,LABEL,"Uses λ from the top of the screen.",0,NULL); break;
        case 1: field(c,u,SET_TABLEAU); field(c,u,SET_ALPHABET); field(c,u,SET_WEIGHT);
            {
                char kind_label[96]; snprintf(kind_label,sizeof(kind_label),"Tableau kind: %s",tableau_kinds[c->tableau_kind]);
                controls_add(u,CHOOSE_TABLEAU_KIND,CHOICE,kind_label,0,NULL);
            }
            controls_add(u,CHOOSE_STANDARD,CHOICE,c->decreasing?"Standard convention: decreasing":"Standard convention: increasing",0,NULL); break;
        case 2: controls_add(u,0,LABEL,"Hook facts and diagram are shown near the top. Cell-based add/remove operations use the selected-cell field in the jeu de taquin section below.",0,NULL); break;
        case 3: {
            controls_add(u,0,LABEL,"Row RSK inserts left to right and bumps the first strictly greater entry. P and Q update insertion by insertion; highlighted P cells are the current bump path.",0,NULL);
            field(c,u,SET_PERMUTATION); field(c,u,SET_WORD); field(c,u,SET_BIWORD); field(c,u,SET_MATRIX);
            controls_add(u,0,LABEL,"Inverse RSK uses standard P and Q below. Promotion and evacuation also use P.",0,NULL);
            controls_add(u,0,LABEL,"P: ordinary standard tableau, rows separated by ;",0,NULL);
            controls_add(u,SET_TABLEAU,FIELD,c->fields[SET_TABLEAU],0,NULL);
            controls_add(u,0,LABEL,"Q: ordinary standard recording tableau",0,NULL);
            controls_add(u,SET_RECORDING,FIELD,c->fields[SET_RECORDING],0,NULL);
            const int ids[]={RSK_START,RSK_PREV,RSK_NEXT,RSK_END}; const char *labels[]={"Start","Prev","Next","End"};
            button_strip(u,ids,labels,4); break;
        }
        case 4: {
            controls_add(u,0,LABEL,"λ is the outer shape. μ is the inner shape. Enter only the visible skew cells in each row. Step moves one entry into the hole.",0,NULL);
            field(c,u,SET_MU); field(c,u,SET_SKEW_TABLEAU); field(c,u,SET_CELL);
            if(c->jeu_loaded) diagram(u,"jeu de taquin board",&c->jeu_tiles);
            const int ids[]={JDT_RESET,JDT_STEP,JDT_SLIDE,JDT_RECTIFY}; const char *labels[]={"Reset","Step","Slide","Rectify"};
            button_strip(u,ids,labels,4); break;
        }
        case 5:
            controls_add(u,0,LABEL,"Uses λ above; LR uses outer ν ÷ inner λ and content μ. The μ field is immediately above.",0,NULL);
            field(c,u,SET_NU); break;
        case 6: controls_add(u,0,LABEL,"Ordinary complex characters and dimensions. Uses λ and the permutation above.",0,NULL); break;
        case 7:
            controls_add(u,0,LABEL,"Exact degree ≤ 8. Basis codes: 0 Schur, 1 power-sum, 2 monomial, 3 complete, 4 elementary.",0,NULL);
            controls_add(u,0,LABEL,"Function: basis; coefficient; partition; coefficient; partition … Coefficient may be numerator,denominator. Example: 0;1;2,1",0,NULL);
            controls_add(u,SET_COEFFICIENTS,FIELD,c->fields[SET_COEFFICIENTS],0,NULL);
            controls_add(u,0,LABEL,"Destination basis code",0,NULL); controls_add(u,SET_BASIS,FIELD,c->fields[SET_BASIS],0,NULL);
            controls_add(u,0,LABEL,"Specialization: finite integer alphabet, e.g. 1,2",0,NULL); controls_add(u,SET_VARIABLES,FIELD,c->fields[SET_VARIABLES],0,NULL);
            controls_add(u,0,LABEL,"Inner function for plethysm, same grammar",0,NULL); controls_add(u,SET_INNER_FUNCTION,FIELD,c->fields[SET_INNER_FUNCTION],0,NULL); break;
        case 8: controls_add(u,0,LABEL,"Start λ, end ν; paths encode box additions. At most 128 paths and 16 edges.",0,NULL); break;
        case 9:
            field(c,u,SET_N); controls_add(u,STEP_MINUS,BUTTON,"n − 1",0,NULL); controls_add(u,STEP_PLUS,BUTTON,"n + 1",0,NULL);
            controls_add(u,0,LABEL,"Uniform permutation / uniform SYT of λ / Plancherel shape of size n. Seed edits restart the reproducible stream.",0,NULL);
            controls_add(u,SET_SEED,FIELD,c->fields[SET_SEED],0,NULL); break;
        case 10: field(c,u,SET_SECOND_PERMUTATION); controls_add(u,0,LABEL,"Reduced word acts on positions from left to right. Bruhat is the strong order on equal-size permutations.",0,NULL); break;
        case 11:
            controls_add(u,CHOOSE_ORIENTATION,CHOICE,c->french?"Diagram orientation: French":"Diagram orientation: English",0,NULL);
            break;
        }
        for(int op=0;op<OP_COUNT;++op)
            if(operation_info[op].section==s && operation_exposed((Operation)op))
                controls_add(u,OP_BASE+op,BUTTON,operation_info[op].label,0,NULL);
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
    case RSK_START: c->rsk_step=0; refresh_rsk(c); break;
    case RSK_PREV: if(c->rsk_step>0) --c->rsk_step; refresh_rsk(c); break;
    case RSK_NEXT: if(c->rsk_step<c->rsk_total) ++c->rsk_step; refresh_rsk(c); break;
    case RSK_END: c->rsk_step=YT_DIM; refresh_rsk(c); break;
    case JDT_RESET: refresh_jeu(c); break;
    case JDT_STEP: {
        if(!c->jeu_ok) { refresh_jeu(c); if(!c->jeu_ok) break; }
        if(!c->jeu.active) {
            Cell cell;
            if(selected_cell(c->fields[SET_CELL],&cell)!=YT_OK || jeu_begin(&c->jeu,cell)!=YT_OK) {
                snprintf(c->output[4],UI_TEXT,"Selected cell must be a removable inner corner of μ."); break;
            }
        }
        JeuStepResult result=jeu_step(&c->jeu);
        if(result==JEU_INVALID) { snprintf(c->output[4],UI_TEXT,"INVALID jeu de taquin state"); c->jeu_ok=false; }
        else project_jeu(c);
        break;
    }
    case JDT_SLIDE: console_run(c,JeuDeTaquinSlide); break;
    case JDT_RECTIFY: console_run(c,Rectify); break;
    case CHOOSE_ORIENTATION: c->french=!c->french; break;
    case CHOOSE_STANDARD: c->decreasing=!c->decreasing; refresh_tableau(c); break;
    case CHOOSE_INSERTION: c->insertion=ROW_INSERTION; refresh_rsk(c); break;
    case CHOOSE_CONTENT: c->content_convention=!c->content_convention; break;
    case CHOOSE_TABLEAU_KIND: c->tableau_kind=(c->tableau_kind+1)%5; refresh_tableau(c); break;
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
static const char *key_labels[]={"1","2","3","4","5","6","7","8","9","0",",",";","−","SPACE","DEL","CLEAR","[","]",".","DONE"};
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
    if(key==14) {
        if(len) { --len; while(len && ((unsigned char)text[len]&0xc0U)==0x80U) --len; text[len]=0; }
    }
    else if(key==15) text[0]=0;
    else {
        const char *add=key==13?" ":key_labels[key]; size_t length=strlen(add);
        if(len+length<512) memcpy(text+len,add,length+1);
        else { snprintf(c->output[11],UI_TEXT,"INPUT LIMIT: 511 characters"); return; }
    }
    if(u->focus==SET_LAMBDA) {
        c->diagram.addition_count=0; refresh_partition(c); refresh_tableau(c); refresh_jeu(c); c->scripted_facts[0]=0;
    }
    if(u->focus==SET_TABLEAU) refresh_tableau(c);
    if(u->focus==SET_SEED) {
        int seed;
        if(integer_field(c->fields[SET_SEED],&seed)==YT_OK && seed>=0) c->rng.state=(uint64_t)seed;
        else snprintf(c->output[9],UI_TEXT,"INVALID seed: enter a nonnegative integer.");
    }
    if(u->focus==SET_PERMUTATION) { c->rsk_input_kind=RSK_PERMUTATION_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); }
    if(u->focus==SET_WORD) { c->rsk_input_kind=RSK_WORD_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); }
    if(u->focus==SET_BIWORD) { c->rsk_input_kind=RSK_BIWORD_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); }
    if(u->focus==SET_MATRIX) { c->rsk_input_kind=RSK_MATRIX_INPUT; c->rsk_step=YT_DIM; refresh_rsk(c); }
    if(u->focus==SET_MU || u->focus==SET_SKEW_TABLEAU) refresh_jeu(c);
}
