#include "advanced.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <ctype.h>
static bool same_partition(const Partition *a,const Partition *b)
{ return a->count==b->count && !memcmp(a->rows,b->rows,(size_t)a->count*sizeof(int)); }
static int partition_row_local(const Partition *p,int row) { return row<p->count?p->rows[row]:0; }
static bool partition_contains(const Partition *outer,const Partition *inner)
{ if(inner->count>outer->count) return false; for(int r=0;r<inner->count;++r) if(inner->rows[r]>outer->rows[r]) return false; return true; }
typedef struct {
 Partition inner,content,outer; Cell cells[YT_CELLS]; int cell_count;
 int used[YT_DIM+1],target[YT_DIM+1]; SkewTableau current; uint64_t count;
 bool overflow; SkewTableauList *list; bool list_overflow; bool impossible;
} LRState;
static bool lr_prefix_lattice(const LRState *s)
{ for(int v=1;v<YT_DIM;++v) if(s->used[v]<s->used[v+1]) return false; return true; }
static void lr_search(LRState *state,int index)
{
    if(state->overflow || state->impossible) return;
    if(index==state->cell_count) {
        if(state->count==UINT64_MAX) { state->overflow=true; return; }
        ++state->count;
        if(state->list) {
            if(state->list->count>=YT_RESULT_TABLEAUX) state->list_overflow=true;
            else state->list->values[state->list->count++]=state->current;
        }
        return;
    }
    Cell cell=state->cells[index];
    int row=cell.row-1,column=cell.column-1;
    if(row<0 || row>=YT_DIM || column<0 || column>=YT_DIM) { state->overflow=true; return; }
    int right=0;
    if(column+1<state->outer.rows[row]
       && column+1>=partition_row_local(&state->inner,row))
        right=state->current.entries[row][column+1];
    int above=0;
    if(row>0 && column>=partition_row_local(&state->inner,row-1)
       && column<state->outer.rows[row-1])
        above=state->current.entries[row-1][column];
    for(int value=1;value<=state->content.count;++value) {
        if(state->used[value]>=state->target[value]) continue;
        if(right && value>right) continue;
        if(above && above>=value) continue;
        state->current.entries[row][column]=value;
        ++state->used[value];
        if(lr_prefix_lattice(state)) lr_search(state,index+1);
        --state->used[value];
        state->current.entries[row][column]=0;
    }
}

static MathStatus lr_prepare(const Partition *inner,const Partition *content,
                             const Partition *outer,LRState *state)
{
    if(!inner || !content || !outer || !state) return YT_MALFORMED;
    MathStatus status=partition_validate(inner); if(status!=YT_OK) return status;
    status=partition_validate(content); if(status!=YT_OK) return status;
    status=partition_validate(outer); if(status!=YT_OK) return status;
    memset(state,0,sizeof(*state));
    if(!partition_contains(outer,inner)) { state->impossible=true; return YT_OK; }
    int skew_size=partition_size(outer)-partition_size(inner);
    if(partition_size(content)!=skew_size) { state->impossible=true; return YT_OK; }
    if(skew_size>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    memset(state,0,sizeof(*state));
    state->inner=*inner; state->content=*content; state->outer=*outer;
    state->current.shape.outer=*outer; state->current.shape.inner=*inner;
    for(int value=1;value<=content->count;++value) state->target[value]=content->rows[value-1];
    /* Reverse row word: top to bottom, right to left. */
    for(int row=0;row<outer->count;++row) {
        int start=partition_row_local(inner,row);
        for(int column=outer->rows[row]-1;column>=start;--column)
            state->cells[state->cell_count++]=(Cell){row+1,column+1};
    }
    return YT_OK;
}

MathStatus littlewood_richardson_coefficient(const Partition *inner,
                                              const Partition *content,
                                              const Partition *outer,
                                              uint64_t *out)
{
    if(!out) return YT_MALFORMED;
    LRState state;
    MathStatus status=lr_prepare(inner,content,outer,&state);
    if(status!=YT_OK) return status;
    lr_search(&state,0);
    if(state.overflow) return YT_OVERFLOW;
    *out=state.count;
    return YT_OK;
}

MathStatus littlewood_richardson_tableaux(const Partition *inner,
                                           const Partition *content,
                                           const Partition *outer,
                                           SkewTableauList *out)
{
    if(!out) return YT_MALFORMED;
    LRState state;
    MathStatus status=lr_prepare(inner,content,outer,&state);
    if(status!=YT_OK) return status;
    SkewTableauList result={0};
    state.list=&result;
    lr_search(&state,0);
    if(state.overflow) return YT_OVERFLOW;
    if(state.list_overflow) return YT_COMPLEXITY;
    *out=result;
    return YT_OK;
}

/* ---------- partition enumeration ---------- */

typedef bool (*PartitionVisitor)(const Partition *partition,void *context);

static bool enumerate_partition_rec(int remaining,int maximum,Partition *current,
                                    PartitionVisitor visitor,void *context)
{
    if(!remaining) return visitor(current,context);
    if(current->count>=YT_DIM) return true;
    int top=remaining<maximum?remaining:maximum;
    for(int part=top;part>=1;--part) {
        current->rows[current->count++]=part;
        if(!enumerate_partition_rec(remaining-part,part,current,visitor,context)) {
            --current->count; return false;
        }
        --current->count;
    }
    return true;
}

static bool enumerate_partitions(int n,PartitionVisitor visitor,void *context)
{
    if(n<0 || n>YT_CELLS) return false;
    if(n==0) { Partition empty={0}; return visitor(&empty,context); }
    Partition current={0};
    return enumerate_partition_rec(n,n,&current,visitor,context);
}

static uint64_t gcd_u64(uint64_t a,uint64_t b)
{
    while(b) { uint64_t t=a%b; a=b; b=t; }
    return a;
}

static uint64_t factorial_small(int n,bool *overflow)
{
    uint64_t value=1;
    for(int k=2;k<=n;++k) {
        if(value>UINT64_MAX/(uint64_t)k) { *overflow=true; return 0; }
        value*= (uint64_t)k;
    }
    return value;
}

static uint64_t z_partition(const Partition *partition,bool *overflow)
{
    uint64_t value=1;
    for(int i=0;i<partition->count;) {
        int part=partition->rows[i],multiplicity=0;
        while(i<partition->count && partition->rows[i]==part) { ++multiplicity; ++i; }
        for(int k=0;k<multiplicity;++k) {
            if(value>UINT64_MAX/(uint64_t)part) { *overflow=true; return 0; }
            value*= (uint64_t)part;
        }
        uint64_t fact=factorial_small(multiplicity,overflow);
        if(*overflow || value>UINT64_MAX/fact) { *overflow=true; return 0; }
        value*=fact;
    }
    return value;
}

static bool skew_border_strip(const Partition *outer,const Partition *inner,
                              int expected,int *height)
{
    if(!partition_contains(outer,inner)) return false;
    Cell cells[YT_CELLS]; int count=0;
    bool occupied[YT_DIM][YT_DIM]={{false}};
    int min_row=YT_DIM,max_row=-1;
    for(int row=0;row<outer->count;++row) {
        int start=partition_row_local(inner,row);
        for(int column=start;column<outer->rows[row];++column) {
            if(count>=YT_CELLS) return false;
            cells[count++]=(Cell){row,column};
            occupied[row][column]=true;
            if(row<min_row) min_row=row;
            if(row>max_row) max_row=row;
        }
    }
    if(count!=expected || !count) return false;
    for(int row=0;row<YT_DIM-1;++row) for(int column=0;column<YT_DIM-1;++column)
        if(occupied[row][column] && occupied[row+1][column]
           && occupied[row][column+1] && occupied[row+1][column+1]) return false;
    bool reached[YT_CELLS]={false};
    int queue[YT_CELLS],head=0,tail=1; queue[0]=0; reached[0]=true;
    while(head<tail) {
        Cell a=cells[queue[head++]];
        for(int i=0;i<count;++i) if(!reached[i]) {
            Cell b=cells[i];
            int distance=abs(a.row-b.row)+abs(a.column-b.column);
            if(distance==1) { reached[i]=true; queue[tail++]=i; }
        }
    }
    if(tail!=count) return false;
    if(height) *height=max_row-min_row+1;
    return true;
}

typedef struct {
    const Partition *outer;
    int target_size,strip_size;
    int64_t sum;
    const int *cycles;
    int cycle_count,index;
    bool overflow;
} CharacterContext;

static int64_t character_rec(const Partition *shape,const int *cycles,int cycle_count,int index,bool *overflow);

static void character_subpartition_rec(const Partition *outer,int row,int previous,int remaining,
                                       Partition *current,CharacterContext *context)
{
    if(context->overflow) return;
    if(row==outer->count) {
        if(remaining) return;
        while(current->count && !current->rows[current->count-1]) --current->count;
        int height=0;
        if(!skew_border_strip(outer,current,context->strip_size,&height)) return;
        int64_t sub=character_rec(current,context->cycles,context->cycle_count,
                                  context->index+1,&context->overflow);
        if(context->overflow) return;
        if(!(height&1)) sub=-sub;
        if((sub>0 && context->sum>INT64_MAX-sub) || (sub<0 && context->sum<INT64_MIN-sub)) {
            context->overflow=true; return;
        }
        context->sum+=sub;
        return;
    }
    int max=outer->rows[row];
    if(max>previous) max=previous;
    if(max>remaining) max=remaining;
    for(int length=max;length>=0;--length) {
        int rows_left=outer->count-row-1;
        (void)rows_left;
        current->rows[row]=length;
        if(length>0 && row+1>current->count) current->count=row+1;
        character_subpartition_rec(outer,row+1,length,remaining-length,current,context);
        if(context->overflow) return;
    }
}

static int64_t character_rec(const Partition *shape,const int *cycles,int cycle_count,int index,bool *overflow)
{
    if(*overflow) return 0;
    if(index==cycle_count) return partition_size(shape)==0?1:0;
    int strip=cycles[index];
    int target=partition_size(shape)-strip;
    if(target<0) return 0;
    Partition current={0};
    CharacterContext context={shape,target,strip,0,cycles,cycle_count,index,false};
    character_subpartition_rec(shape,0,shape->count?shape->rows[0]:0,target,&current,&context);
    if(context.overflow) *overflow=true;
    return context.sum;
}

MathStatus symmetric_character_cycle_type(const Partition *shape,
                                           const Partition *cycle_type,
                                           int64_t *out)
{
    if(!shape || !cycle_type || !out) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    status=partition_validate(cycle_type); if(status!=YT_OK) return status;
    int n=partition_size(shape);
    if(partition_size(cycle_type)!=n) return YT_MALFORMED;
    if(n>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    int cycles[YT_DIM];
    for(int i=0;i<cycle_type->count;++i) cycles[i]=cycle_type->rows[i];
    bool overflow=false;
    int64_t value=character_rec(shape,cycles,cycle_type->count,0,&overflow);
    if(overflow) return YT_OVERFLOW;
    *out=value;
    return YT_OK;
}

typedef struct {
    Partition shape,content;
    int target[YT_DIM+1],used[YT_DIM+1];
    Tableau tableau;
    uint64_t count;
    bool overflow;
} KostkaState;

static void kostka_search(KostkaState *state,int linear)
{
    if(state->overflow) return;
    int total=partition_size(&state->shape);
    if(linear==total) {
        if(state->count==UINT64_MAX) { state->overflow=true; return; }
        ++state->count; return;
    }
    int offset=0,row=0;
    while(row<state->shape.count && linear>=offset+state->shape.rows[row]) {
        offset+=state->shape.rows[row]; ++row;
    }
    int column=linear-offset;
    int left=column?state->tableau.entries[row][column-1]:0;
    int above=(row && column<state->shape.rows[row-1])?state->tableau.entries[row-1][column]:0;
    for(int value=1;value<=state->content.count;++value) {
        if(state->used[value]>=state->target[value]) continue;
        if(left && left>value) continue;
        if(above && above>=value) continue;
        state->tableau.entries[row][column]=value;
        ++state->used[value];
        kostka_search(state,linear+1);
        --state->used[value];
        state->tableau.entries[row][column]=0;
    }
}

static MathStatus kostka_number(const Partition *shape,const Partition *content,uint64_t *out)
{
    if(!shape || !content || !out) return YT_MALFORMED;
    MathStatus status=partition_validate(shape); if(status!=YT_OK) return status;
    status=partition_validate(content); if(status!=YT_OK) return status;
    if(partition_size(shape)!=partition_size(content)) return YT_MALFORMED;
    if(partition_size(shape)>YT_ADVANCED_DEGREE_MAX) return YT_LIMIT;
    KostkaState state={0}; state.shape=*shape; state.content=*content; state.tableau.shape=*shape;
    for(int value=1;value<=content->count;++value) state.target[value]=content->rows[value-1];
    kostka_search(&state,0);
    if(state.overflow) return YT_OVERFLOW;
    *out=state.count; return YT_OK;
}

static bool collect_partition_visit(const Partition *partition,void *raw)
{
    PartitionList *list=raw;
    if(list->count>=YT_RESULT_PARTITIONS) return false;
    list->values[list->count++]=*partition;
    return true;
}

static MathStatus collect_partitions(int n,PartitionList *out)
{
    if(!out || n<0) return YT_MALFORMED;
    if(n>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
    PartitionList result={0};
    if(!enumerate_partitions(n,collect_partition_visit,&result)) return YT_LIMIT;
    *out=result; return YT_OK;
}

typedef SymmetricFunction PowerExpansion;
typedef SymmetricTerm PowerTerm;
static uint64_t abs_i64(int64_t value) { return value<0?(uint64_t)(-(value+1))+1U:(uint64_t)value; }
static void rational_normalize(Rational *value)
{
    if(!value->numerator) { value->denominator=1; return; }
    uint64_t g=gcd_u64(abs_i64(value->numerator),value->denominator);
    value->numerator/=(int64_t)g; value->denominator/=g;
}

static bool mul_i64(int64_t a,int64_t b,int64_t *out)
{
    if(!a || !b) { *out=0; return true; }
    if(a==INT64_MIN && b==-1) return false;
    if(b==INT64_MIN && a==-1) return false;
    if(a>0) {
        if(b>0 && a>INT64_MAX/b) return false;
        if(b<0 && b<INT64_MIN/a) return false;
    } else {
        if(b>0 && a<INT64_MIN/b) return false;
        if(b<0 && a!=0 && b<INT64_MAX/a) return false;
    }
    *out=a*b; return true;
}

static bool rational_multiply(Rational left,Rational right,Rational *out)
{
    uint64_t g1=gcd_u64(abs_i64(left.numerator),right.denominator);
    uint64_t g2=gcd_u64(abs_i64(right.numerator),left.denominator);
    int64_t a=left.numerator/(int64_t)g1,b=right.numerator/(int64_t)g2;
    uint64_t d1=left.denominator/g2,d2=right.denominator/g1;
    int64_t numerator;
    if(!mul_i64(a,b,&numerator) || numerator==INT64_MIN || d1>(uint64_t)INT64_MAX/d2) return false;
    *out=(Rational){numerator,d1*d2}; rational_normalize(out); return true;
}

static bool rational_add(Rational left,Rational right,Rational *out)
{
    uint64_t g=gcd_u64(left.denominator,right.denominator);
    uint64_t lm=right.denominator/g,rm=left.denominator/g;
    int64_t a,b;
    if(lm>(uint64_t)INT64_MAX || rm>(uint64_t)INT64_MAX) return false;
    if(!mul_i64(left.numerator,(int64_t)lm,&a) || !mul_i64(right.numerator,(int64_t)rm,&b)) return false;
    if((b>0 && a>INT64_MAX-b) || (b<0 && a<INT64_MIN-b)) return false;
    if(left.denominator>(uint64_t)INT64_MAX/lm || a+b==INT64_MIN) return false;
    *out=(Rational){a+b,left.denominator*lm}; rational_normalize(out); return true;
}

static void sort_partition(Partition *partition)
{
    for(int i=1;i<partition->count;++i) {
        int value=partition->rows[i],j=i;
        while(j>0 && partition->rows[j-1]<value) { partition->rows[j]=partition->rows[j-1]; --j; }
        partition->rows[j]=value;
    }
}

static bool power_add(PowerExpansion *expansion,const Partition *index,Rational coefficient)
{
    if(!coefficient.numerator) return true;
    for(int i=0;i<expansion->count;++i) if(same_partition(&expansion->terms[i].index,index)) {
        Rational sum;
        if(!rational_add(expansion->terms[i].coefficient,coefficient,&sum)) return false;
        expansion->terms[i].coefficient=sum; return true;
    }
    if(expansion->count>=YT_RESULT_TERMS) return false;
    expansion->terms[expansion->count++]=(PowerTerm){*index,coefficient};
    return true;
}

static bool power_multiply(const PowerExpansion *left,const PowerExpansion *right,PowerExpansion *out)
{
    PowerExpansion result={0};
    for(int i=0;i<left->count;++i) for(int j=0;j<right->count;++j) {
        Partition index=left->terms[i].index;
        if(index.count+right->terms[j].index.count>YT_DIM) return false;
        for(int k=0;k<right->terms[j].index.count;++k) index.rows[index.count++]=right->terms[j].index.rows[k];
        sort_partition(&index);
        Rational coefficient;
        if(!rational_multiply(left->terms[i].coefficient,right->terms[j].coefficient,&coefficient)) return false;
        if(!power_add(&result,&index,coefficient)) return false;
    }
    *out=result; return true;
}

static MathStatus schur_power_expansion(const Partition *shape,PowerExpansion *out)
{
    int degree=partition_size(shape);
    PartitionList partitions;
    MathStatus status=collect_partitions(degree,&partitions); if(status!=YT_OK) return status;
    PowerExpansion result={0};
    for(int i=0;i<partitions.count;++i) {
        int64_t character=0;
        status=symmetric_character_cycle_type(shape,&partitions.values[i],&character);
        if(status!=YT_OK) return status;
        if(!character) continue;
        bool overflow=false; uint64_t z=z_partition(&partitions.values[i],&overflow);
        if(overflow || !z) return YT_OVERFLOW;
        if(!power_add(&result,&partitions.values[i],(Rational){character,z})) return YT_OVERFLOW;
    }
    *out=result; return YT_OK;
}


/* All basis conversions pass through exact power sums. Rows are power indices;
 * columns are source basis elements in decreasing lexicographic partition order. */
#define BASIS_MATRIX_MAX 22
static MathStatus function_validate(const SymmetricFunction *input)
{
    if(!input || input->count<0 || input->count>YT_RESULT_TERMS || input->basis<BASIS_SCHUR || input->basis>BASIS_ELEMENTARY) return YT_MALFORMED;
    for(int index=0;index<input->count;++index) {
        MathStatus status=partition_validate(&input->terms[index].index);
        if(status!=YT_OK) return status;
        if(partition_size(&input->terms[index].index)>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
        if(!input->terms[index].coefficient.denominator || input->terms[index].coefficient.denominator>(uint64_t)INT64_MAX || input->terms[index].coefficient.numerator==INT64_MIN) return YT_MALFORMED;
    }
    return YT_OK;
}
static MathStatus matrix_inverse(int count,Rational matrix[BASIS_MATRIX_MAX][BASIS_MATRIX_MAX],Rational inverse[BASIS_MATRIX_MAX][BASIS_MATRIX_MAX])
{
    for(int row=0;row<count;++row) for(int column=0;column<count;++column)
        inverse[row][column]=(Rational){row==column?1:0,1};
    for(int column=0;column<count;++column) {
        int pivot=column;
        while(pivot<count && !matrix[pivot][column].numerator) ++pivot;
        if(pivot==count) return YT_MALFORMED;
        if(pivot!=column) for(int index=0;index<count;++index) {
            Rational swap=matrix[column][index]; matrix[column][index]=matrix[pivot][index]; matrix[pivot][index]=swap;
            swap=inverse[column][index]; inverse[column][index]=inverse[pivot][index]; inverse[pivot][index]=swap;
        }
        Rational value=matrix[column][column],reciprocal;
        if(value.denominator>(uint64_t)INT64_MAX || value.numerator==INT64_MIN) return YT_OVERFLOW;
        reciprocal=(Rational){value.numerator<0?-(int64_t)value.denominator:(int64_t)value.denominator,abs_i64(value.numerator)};
        for(int index=0;index<count;++index)
            if(!rational_multiply(matrix[column][index],reciprocal,&matrix[column][index]) || !rational_multiply(inverse[column][index],reciprocal,&inverse[column][index])) return YT_OVERFLOW;
        for(int row=0;row<count;++row) if(row!=column) {
            Rational factor=matrix[row][column];
            if(factor.numerator==INT64_MIN) return YT_OVERFLOW;
            factor.numerator=-factor.numerator;
            for(int index=0;index<count;++index) {
                Rational product;
                if(!rational_multiply(factor,matrix[column][index],&product) || !rational_add(matrix[row][index],product,&matrix[row][index])) return YT_OVERFLOW;
                if(!rational_multiply(factor,inverse[column][index],&product) || !rational_add(inverse[row][index],product,&inverse[row][index])) return YT_OVERFLOW;
            }
        }
    }
    return YT_OK;
}
static MathStatus basis_element_power(SymmetricBasis basis,const Partition *index,PowerExpansion *out)
{
    PowerExpansion result={.basis=BASIS_POWER};
    if(basis==BASIS_POWER) {
        result.count=1; result.terms[0]=(PowerTerm){*index,{1,1}}; *out=result; return YT_OK;
    }
    if(basis==BASIS_SCHUR) return schur_power_expansion(index,out);
    if(basis==BASIS_COMPLETE || basis==BASIS_ELEMENTARY) {
        result.count=1; result.terms[0]=(PowerTerm){{0},{1,1}};
        for(int part=0;part<index->count;++part) {
            PartitionList cycles; MathStatus status=collect_partitions(index->rows[part],&cycles);
            if(status!=YT_OK) return status;
            PowerExpansion factor={.basis=BASIS_POWER},next;
            for(int item=0;item<cycles.count;++item) {
                bool overflow=false; uint64_t denominator=z_partition(&cycles.values[item],&overflow);
                if(overflow) return YT_OVERFLOW;
                int sign=basis==BASIS_ELEMENTARY && ((index->rows[part]-cycles.values[item].count)&1)?-1:1;
                if(!power_add(&factor,&cycles.values[item],(Rational){sign,denominator})) return YT_OVERFLOW;
            }
            if(!power_multiply(&result,&factor,&next)) return YT_OVERFLOW;
            result=next;
        }
        *out=result; return YT_OK;
    }
    if(basis!=BASIS_MONOMIAL) return YT_MALFORMED;
    PartitionList partitions; MathStatus status=collect_partitions(partition_size(index),&partitions);
    if(status!=YT_OK) return status;
    int count=partitions.count,selected=-1;
    Rational kostka[BASIS_MATRIX_MAX][BASIS_MATRIX_MAX],inverse[BASIS_MATRIX_MAX][BASIS_MATRIX_MAX];
    for(int row=0;row<count;++row) {
        if(same_partition(index,&partitions.values[row])) selected=row;
        for(int column=0;column<count;++column) {
            uint64_t coefficient;
            status=kostka_number(&partitions.values[row],&partitions.values[column],&coefficient);
            if(status!=YT_OK) return status;
            if(coefficient>(uint64_t)INT64_MAX) return YT_OVERFLOW;
            kostka[row][column]=(Rational){(int64_t)coefficient,1};
        }
    }
    if(selected<0) return YT_MALFORMED;
    status=matrix_inverse(count,kostka,inverse); if(status!=YT_OK) return status;
    for(int column=0;column<count;++column) if(inverse[selected][column].numerator) {
        PowerExpansion schur;
        status=schur_power_expansion(&partitions.values[column],&schur); if(status!=YT_OK) return status;
        for(int term=0;term<schur.count;++term) {
            Rational coefficient;
            if(!rational_multiply(inverse[selected][column],schur.terms[term].coefficient,&coefficient) || !power_add(&result,&schur.terms[term].index,coefficient)) return YT_OVERFLOW;
        }
    }
    *out=result; return YT_OK;
}
static void compact(PowerExpansion *function)
{
    int count=0;
    for(int index=0;index<function->count;++index) if(function->terms[index].coefficient.numerator)
        function->terms[count++]=function->terms[index];
    function->count=count;
}
static MathStatus to_power(const SymmetricFunction *input,PowerExpansion *out)
{
    MathStatus status=function_validate(input); if(status!=YT_OK) return status;
    PowerExpansion result={.basis=BASIS_POWER};
    for(int index=0;index<input->count;++index) {
        PowerExpansion element;
        status=basis_element_power(input->basis,&input->terms[index].index,&element); if(status!=YT_OK) return status;
        for(int term=0;term<element.count;++term) {
            Rational coefficient;
            if(!rational_multiply(element.terms[term].coefficient,input->terms[index].coefficient,&coefficient) || !power_add(&result,&element.terms[term].index,coefficient)) return YT_OVERFLOW;
        }
    }
    compact(&result); *out=result; return YT_OK;
}
MathStatus symmetric_change_basis(const SymmetricFunction *input,SymmetricBasis basis,SymmetricFunction *out)
{
    if(!out || basis<BASIS_SCHUR || basis>BASIS_ELEMENTARY) return YT_MALFORMED;
    PowerExpansion power;
    MathStatus status=to_power(input,&power); if(status!=YT_OK) return status;
    if(basis==BASIS_POWER) { *out=power; return YT_OK; }
    SymmetricFunction result={.basis=basis};
    for(int degree=0;degree<=YT_ADVANCED_DEGREE_MAX;++degree) {
        bool needed=false;
        for(int term=0;term<power.count;++term) if(partition_size(&power.terms[term].index)==degree) needed=true;
        if(!needed) continue;
        PartitionList partitions; status=collect_partitions(degree,&partitions); if(status!=YT_OK) return status;
        int count=partitions.count;
        Rational matrix[BASIS_MATRIX_MAX][BASIS_MATRIX_MAX],inverse[BASIS_MATRIX_MAX][BASIS_MATRIX_MAX],vector[BASIS_MATRIX_MAX];
        for(int row=0;row<count;++row) {
            vector[row]=(Rational){0,1};
            for(int term=0;term<power.count;++term) if(same_partition(&partitions.values[row],&power.terms[term].index)) vector[row]=power.terms[term].coefficient;
            for(int column=0;column<count;++column) matrix[row][column]=(Rational){0,1};
        }
        for(int column=0;column<count;++column) {
            PowerExpansion element;
            status=basis_element_power(basis,&partitions.values[column],&element); if(status!=YT_OK) return status;
            for(int row=0;row<count;++row) for(int term=0;term<element.count;++term)
                if(same_partition(&partitions.values[row],&element.terms[term].index)) matrix[row][column]=element.terms[term].coefficient;
        }
        status=matrix_inverse(count,matrix,inverse); if(status!=YT_OK) return status;
        for(int row=0;row<count;++row) {
            Rational sum={0,1};
            for(int column=0;column<count;++column) {
                Rational product;
                if(!rational_multiply(inverse[row][column],vector[column],&product) || !rational_add(sum,product,&sum)) return YT_OVERFLOW;
            }
            if(!power_add(&result,&partitions.values[row],sum)) return YT_OVERFLOW;
        }
    }
    compact(&result); *out=result; return YT_OK;
}
MathStatus symmetric_schur_product(const Partition *left,const Partition *right,SymmetricFunction *out)
{
    if(!out) return YT_MALFORMED;
    MathStatus status=partition_validate(left); if(status!=YT_OK) return status;
    status=partition_validate(right); if(status!=YT_OK) return status;
    PartitionList partitions;
    status=collect_partitions(partition_size(left)+partition_size(right),&partitions); if(status!=YT_OK) return status;
    SymmetricFunction result={.basis=BASIS_SCHUR};
    for(int index=0;index<partitions.count;++index) if(partition_contains(&partitions.values[index],left)) {
        uint64_t coefficient;
        status=littlewood_richardson_coefficient(left,right,&partitions.values[index],&coefficient);
        if(status!=YT_OK) return status;
        if(coefficient>(uint64_t)INT64_MAX || !power_add(&result,&partitions.values[index],(Rational){(int64_t)coefficient,1})) return YT_OVERFLOW;
    }
    *out=result; return YT_OK;
}
MathStatus symmetric_plethysm(const SymmetricFunction *outer,const SymmetricFunction *inner,SymmetricFunction *out)
{
    if(!out) return YT_MALFORMED;
    PowerExpansion first,second;
    MathStatus status=to_power(outer,&first); if(status!=YT_OK) return status;
    status=to_power(inner,&second); if(status!=YT_OK) return status;
    PowerExpansion total={.basis=BASIS_POWER};
    for(int term=0;term<first.count;++term) {
        PowerExpansion product={.count=1,.basis=BASIS_POWER};
        product.terms[0]=(PowerTerm){{0},first.terms[term].coefficient};
        for(int part=0;part<first.terms[term].index.count;++part) {
            int scale=first.terms[term].index.rows[part];
            PowerExpansion factor={.basis=BASIS_POWER},next;
            for(int index=0;index<second.count;++index) {
                Partition scaled=second.terms[index].index;
                if(partition_size(&scaled)*scale>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
                for(int row=0;row<scaled.count;++row) scaled.rows[row]*=scale;
                if(!power_add(&factor,&scaled,second.terms[index].coefficient)) return YT_OVERFLOW;
            }
            if(!power_multiply(&product,&factor,&next)) return YT_OVERFLOW;
            for(int index=0;index<next.count;++index) if(partition_size(&next.terms[index].index)>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
            product=next;
        }
        for(int index=0;index<product.count;++index)
            if(!power_add(&total,&product.terms[index].index,product.terms[index].coefficient)) return YT_OVERFLOW;
    }
    compact(&total); total.basis=BASIS_POWER;
    return symmetric_change_basis(&total,BASIS_SCHUR,out);
}
MathStatus symmetric_specialize(const SymmetricFunction *input,const int *alphabet,int count,Rational *out)
{
    if(!out || count<0 || count>YT_DIM || (count && !alphabet)) return YT_MALFORMED;
    PowerExpansion power;
    MathStatus status=to_power(input,&power); if(status!=YT_OK) return status;
    Rational result={0,1};
    for(int index=0;index<power.count;++index) {
        Rational term=power.terms[index].coefficient;
        for(int part=0;part<power.terms[index].index.count;++part) {
            int64_t sum=0;
            for(int letter=0;letter<count;++letter) {
                int64_t value=1;
                for(int exponent=0;exponent<power.terms[index].index.rows[part];++exponent)
                    if(!mul_i64(value,alphabet[letter],&value)) return YT_OVERFLOW;
                if((value>0 && sum>INT64_MAX-value) || (value<0 && sum<INT64_MIN-value)) return YT_OVERFLOW;
                sum+=value;
            }
            if(!rational_multiply(term,(Rational){sum,1},&term)) return YT_OVERFLOW;
        }
        if(!rational_add(result,term,&result)) return YT_OVERFLOW;
    }
    *out=result; return YT_OK;
}
static bool read_piece(const char **cursor,char piece[512])
{
    size_t length=strcspn(*cursor,";");
    if(length>=512) return false;
    memcpy(piece,*cursor,length); piece[length]=0;
    *cursor+=length;
    if(**cursor==';') ++*cursor;
    return true;
}
static bool read_coefficient(const char *text,Rational *out)
{
    const char *cursor=text;
    while(isspace((unsigned char)*cursor)) ++cursor;
    bool unicode_minus=strncmp(cursor,"−",3)==0;
    if(unicode_minus) { cursor+=3; if(!isdigit((unsigned char)*cursor)) return false; }
    errno=0; char *end;
    int64_t numerator=strtoll(cursor,&end,10);
    if(end==cursor || errno || numerator==INT64_MIN) return false;
    if(unicode_minus) numerator=-numerator;
    cursor=end; while(isspace((unsigned char)*cursor)) ++cursor;
    uint64_t denominator=1;
    if(*cursor==',') {
        ++cursor; while(isspace((unsigned char)*cursor)) ++cursor;
        if(!isdigit((unsigned char)*cursor)) return false;
        errno=0; denominator=strtoull(cursor,&end,10);
        if(end==cursor || errno || !denominator || denominator>(uint64_t)INT64_MAX) return false;
        cursor=end; while(isspace((unsigned char)*cursor)) ++cursor;
    }
    if(*cursor) return false;
    *out=(Rational){numerator,denominator}; rational_normalize(out); return true;
}
MathStatus symmetric_parse(const char *text,SymmetricFunction *out)
{
    if(!text || !out) return YT_MALFORMED;
    size_t length=strlen(text);
    while(length && isspace((unsigned char)text[length-1])) --length;
    if(!length || text[length-1]==';') return YT_MALFORMED;
    const char *cursor=text; char piece[512];
    if(!read_piece(&cursor,piece)) return YT_LIMIT;
    Rational code;
    if(!read_coefficient(piece,&code) || code.denominator!=1 || code.numerator<0 || code.numerator>4) return YT_MALFORMED;
    SymmetricFunction result={.basis=(SymmetricBasis)code.numerator};
    while(*cursor) {
        Rational coefficient; Partition index;
        if(!strchr(cursor,';') || !read_piece(&cursor,piece) || !read_coefficient(piece,&coefficient) || !read_piece(&cursor,piece)) return YT_MALFORMED;
        MathStatus status=partition_parse(piece,&index); if(status!=YT_OK) return status;
        if(partition_size(&index)>YT_ADVANCED_DEGREE_MAX) return YT_COMPLEXITY;
        if(!power_add(&result,&index,coefficient)) return YT_OVERFLOW;
    }
    compact(&result); *out=result; return YT_OK;
}
static bool write_text(char *out,size_t capacity,const char *text)
{
    size_t used=strlen(out),length=strlen(text);
    if(used+length>=capacity) return false;
    memcpy(out+used,text,length+1); return true;
}
MathStatus symmetric_text(const SymmetricFunction *input,char *out,size_t capacity)
{
    MathStatus status=function_validate(input);
    if(status!=YT_OK || !out || !capacity) return status==YT_OK?YT_MALFORMED:status;
    char result[8192]="";
    static const char *symbols[]={"s","p","m","h","e"};
    int emitted=0;
    for(int term=0;term<input->count;++term) {
        Rational coefficient=input->terms[term].coefficient;
        if(!coefficient.numerator) continue;
        if(!write_text(result,sizeof(result),emitted++?(coefficient.numerator<0?" − ":" + "):(coefficient.numerator<0?"−":""))) return YT_LIMIT;
        char number[96];
        if(coefficient.denominator==1) snprintf(number,sizeof(number),"%" PRIu64,abs_i64(coefficient.numerator));
        else snprintf(number,sizeof(number),"%" PRIu64 "÷%" PRIu64,abs_i64(coefficient.numerator),coefficient.denominator);
        if(!write_text(result,sizeof(result),number) || !write_text(result,sizeof(result)," × ") || !write_text(result,sizeof(result),symbols[input->basis]) || !write_text(result,sizeof(result),"[")) return YT_LIMIT;
        for(int row=0;row<input->terms[term].index.count;++row) {
            snprintf(number,sizeof(number),"%s%d",row?",":"",input->terms[term].index.rows[row]);
            if(!write_text(result,sizeof(result),number)) return YT_LIMIT;
        }
        if(!write_text(result,sizeof(result),"]")) return YT_LIMIT;
    }
    if(!emitted) strcpy(result,"0");
    if(strlen(result)>=capacity) return YT_LIMIT;
    strcpy(out,result); return YT_OK;
}
