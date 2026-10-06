#include "young_math.h"
#include <ctype.h>
#include <limits.h>
#include <string.h>

const char *math_status(MathStatus s)
{
    switch (s) {
    case YT_OK: return "OK";
    case YT_MALFORMED: return "INVALID INPUT";
    case YT_LIMIT: return "LIMIT: 64 rows/columns, 256 cells";
    case YT_OVERFLOW: return "OVERFLOW: exact uint64 range exceeded";
    }
    return "ERROR";
}

/* Strict list grammar: optional [], whitespace/comma-separated signed decimals.
 * Semicolons belong only to the tableau row grammar. Never accept a prefix. */
static void spaces(const char **p) { while (isspace((unsigned char)**p)) ++*p; }
static MathStatus list(const char **p, int *values, int *count, bool row)
{
    *count = 0;
    spaces(p);
    bool bracket = **p == '[';
    if (bracket) { ++*p; spaces(p); }
    if (**p == ']' && bracket) { ++*p; return YT_OK; }
    if (**p == '\0' && !bracket) return YT_OK;
    for (;;) {
        bool negative = **p == '-';
        if (negative || **p == '+') ++*p;
        if (!isdigit((unsigned char)**p)) return YT_MALFORMED;
        uint64_t value = 0;
        while (isdigit((unsigned char)**p)) {
            unsigned digit = (unsigned)(**p - '0');
            uint64_t bound = negative ? (uint64_t)INT_MAX + 1 : INT_MAX;
            if (value > (bound - digit) / 10) return YT_OVERFLOW;
            value = value * 10 + digit;
            ++*p;
        }
        if (*count == YT_DIM) return YT_LIMIT;
        values[(*count)++] = negative ? (value == (uint64_t)INT_MAX + 1 ? INT_MIN : -(int)value) : (int)value;
        bool had_space = isspace((unsigned char)**p) != 0;
        spaces(p);
        if (bracket && **p == ']') { ++*p; return YT_OK; }
        if (!bracket && (**p == '\0' || (row && **p == ';'))) return YT_OK;
        if (**p == ',') { ++*p; spaces(p); }
        else if (!had_space) return YT_MALFORMED;
        if (!isdigit((unsigned char)**p) && **p != '-' && **p != '+') return YT_MALFORMED;
    }
}
MathStatus partition_parse(const char *text, Partition *out)
{
    Partition tmp = {0};
    const char *p = text;
    MathStatus s = list(&p, tmp.rows, &tmp.count, false);
    if (s != YT_OK) return s;
    spaces(&p);
    if (*p) return YT_MALFORMED;
    int size = 0;
    for (int r = 0; r < tmp.count; ++r) {
        if (tmp.rows[r] <= 0 || (r && tmp.rows[r] > tmp.rows[r-1])) return YT_MALFORMED;
        if (tmp.rows[r] > YT_DIM || size > YT_CELLS - tmp.rows[r]) return YT_LIMIT;
        size += tmp.rows[r];
    }
    *out = tmp;
    return YT_OK;
}
int partition_size(const Partition *p)
{ int size = 0; for (int r = 0; r < p->count; ++r) size += p->rows[r]; return size; }
Partition partition_conjugate(const Partition *p)
{
    Partition out = {0};
    if (p->count) out.count = p->rows[0];
    for (int c = 0; c < out.count; ++c)
        for (int r = 0; r < p->count; ++r) out.rows[c] += p->rows[r] > c;
    return out;
}
int partition_cells(const Partition *p, Cell *out)
{
    int n = 0;
    for (int r = 0; r < p->count; ++r)
        for (int c = 0; c < p->rows[r]; ++c) out[n++] = (Cell){r+1,c+1};
    return n;
}
int partition_removable(const Partition *p, Cell *out)
{
    int n = 0;
    for (int r = 0; r < p->count; ++r)
        if (r+1 == p->count || p->rows[r] > p->rows[r+1]) out[n++] = (Cell){r+1,p->rows[r]};
    return n;
}
int partition_addable(const Partition *p, Cell *out)
{
    int n = 0;
    for (int r = 0; r < p->count; ++r)
        if (!r || p->rows[r-1] > p->rows[r]) out[n++] = (Cell){r+1,p->rows[r]+1};
    out[n++] = (Cell){p->count+1,1};
    return n;
}
int partition_hook(const Partition *p, int r, int c)
{
    if (r < 1 || r > p->count || c < 1 || c > p->rows[r-1]) return 0;
    int h = p->rows[r-1] - c + 1;
    for (int i = r; i < p->count; ++i) h += p->rows[i] >= c;
    return h;
}
MathStatus partition_hook_product(const Partition *p, uint64_t *out)
{
    uint64_t value = 1;
    for (int r = 1; r <= p->count; ++r) for (int c = 1; c <= p->rows[r-1]; ++c) {
        uint64_t h = (uint64_t)partition_hook(p,r,c);
        if (value > UINT64_MAX / h) return YT_OVERFLOW;
        value *= h;
    }
    *out = value; return YT_OK;
}
/* Prime cancellation computes n!/product(h) without overflowing n! or the
 * hook product when the final quotient still fits. */
MathStatus partition_standard_count(const Partition *p, uint64_t *out)
{
    int exponents[YT_CELLS+1] = {0};
    int n = partition_size(p);
    for (int k = 2; k <= n; ++k) {
        int v = k;
        for (int d = 2; d <= v; ++d) while (v % d == 0) { ++exponents[d]; v /= d; }
    }
    for (int r = 1; r <= p->count; ++r) for (int c = 1; c <= p->rows[r-1]; ++c) {
        int v = partition_hook(p,r,c);
        for (int d = 2; d <= v; ++d) while (v % d == 0) { --exponents[d]; v /= d; }
    }
    uint64_t value = 1;
    for (int d = 2; d <= n; ++d) {
        if (exponents[d] < 0) return YT_MALFORMED;
        for (int k = 0; k < exponents[d]; ++k) {
            if (value > UINT64_MAX / (unsigned)d) return YT_OVERFLOW;
            value *= (unsigned)d;
        }
    }
    *out = value; return YT_OK;
}
MathStatus tableau_parse(const char *text, Tableau *out)
{
    Tableau tmp = {0}; const char *p = text; int size = 0;
    spaces(&p);
    if (!*p || strcmp(p,"[]") == 0) { *out = tmp; return YT_OK; }
    for (;;) {
        int r = tmp.shape.count;
        if (r == YT_DIM) return YT_LIMIT;
        MathStatus s = list(&p,tmp.entries[r],&tmp.shape.rows[r],true);
        if (s != YT_OK) return s;
        if (!tmp.shape.rows[r]) {
            spaces(&p);
            if (!r && !*p) { *out = tmp; return YT_OK; }
            return YT_MALFORMED;
        }
        size += tmp.shape.rows[r];
        if (size > YT_CELLS) return YT_LIMIT;
        ++tmp.shape.count; spaces(&p);
        if (!*p) break;
        if (*p++ != ';') return YT_MALFORMED;
        spaces(&p); if (!*p) return YT_MALFORMED;
    }
    *out = tmp; return YT_OK;
}
Validation tableau_validate(const Partition *shape, const Tableau *t, bool decreasing)
{
    Validation v = {true,true,true,true};
    int n = partition_size(shape); bool seen[YT_CELLS+1] = {false};
    if (shape->count != t->shape.count) v.shape = false;
    for (int r = 0; r < t->shape.count; ++r) {
        if (r >= shape->count || t->shape.rows[r] != shape->rows[r]) v.shape = false;
        for (int c = 0; c < t->shape.rows[r]; ++c) {
            int entry = t->entries[r][c];
            if (entry < 1 || entry > n || seen[entry]) v.standard = false;
            else seen[entry] = true;
            if (c && (decreasing ? t->entries[r][c-1] <= entry : t->entries[r][c-1] >= entry)) v.rows = false;
            if (r && c < t->shape.rows[r-1] && (decreasing ? t->entries[r-1][c] <= entry : t->entries[r-1][c] >= entry)) v.columns = false;
        }
    }
    for (int i = 1; i <= n; ++i) if (!seen[i]) v.standard = false;
    v.standard = v.standard && v.shape && v.rows && v.columns;
    return v;
}
static MathStatus sequence_parse(const char *text,int *values,int *count,bool permutation)
{
    const char *cursor=text;
    MathStatus status=list(&cursor,values,count,false);
    if(status!=YT_OK) return status;
    spaces(&cursor);
    if(*cursor) return YT_MALFORMED;
    bool seen[YT_DIM+1]={false};
    for(int index=0;index<*count;++index) {
        int value=values[index];
        if(value<1) return YT_MALFORMED;
        if(permutation) {
            if(value>*count || seen[value]) return YT_MALFORMED;
            seen[value]=true;
        }
    }
    return YT_OK;
}

static void rsk_insert(Tableau *p,Tableau *q,int value,int record,Cell *path,int *path_count)
{
    for(int row=0;row<YT_DIM;++row) {
        int column=0;
        while(column<p->shape.rows[row] && p->entries[row][column]<=value) ++column;
        if(path && path_count && *path_count<YT_DIM)
            path[(*path_count)++]=(Cell){row+1,column+1};
        if(column==p->shape.rows[row]) {
            p->entries[row][column]=value;
            q->entries[row][column]=record;
            ++p->shape.rows[row];
            q->shape.rows[row]=p->shape.rows[row];
            if(row==p->shape.count) ++p->shape.count;
            q->shape.count=p->shape.count;
            return;
        }
        int bumped=p->entries[row][column];
        p->entries[row][column]=value;
        value=bumped;
    }
}

static MathStatus rsk_trace(const char *text,bool permutation,int step,RSKTrace *out)
{
    RSKTrace trace={0};
    MathStatus status=sequence_parse(text,trace.values,&trace.count,permutation);
    if(status!=YT_OK) return status;
    if(step<0) step=0;
    if(step>trace.count) step=trace.count;
    trace.step=step;
    trace.complete=step==trace.count;
    for(int index=0;index<step;++index) {
        Cell *path=NULL;
        int *path_count=NULL;
        if(index+1==step) {
            trace.path_count=0;
            path=trace.path;
            path_count=&trace.path_count;
            trace.inserted=trace.values[index];
        }
        rsk_insert(&trace.p,&trace.q,trace.values[index],index+1,path,path_count);
    }
    *out=trace;
    return YT_OK;
}

MathStatus permutation_rsk_trace(const char *text,int step,RSKTrace *out)
{ return rsk_trace(text,true,step,out); }

MathStatus word_rsk_trace(const char *text,int step,RSKTrace *out)
{ return rsk_trace(text,false,step,out); }

MathStatus permutation_rsk(const char *text,Tableau *pout,Tableau *qout)
{
    RSKTrace trace;
    MathStatus status=permutation_rsk_trace(text,YT_DIM,&trace);
    if(status!=YT_OK) return status;
    *pout=trace.p;
    *qout=trace.q;
    return YT_OK;
}

MathStatus word_rsk(const char *text,Tableau *pout,Tableau *qout)
{
    RSKTrace trace;
    MathStatus status=word_rsk_trace(text,YT_DIM,&trace);
    if(status!=YT_OK) return status;
    *pout=trace.p;
    *qout=trace.q;
    return YT_OK;
}

static int partition_row(const Partition *partition,int row)
{ return row>=0 && row<partition->count?partition->rows[row]:0; }

static bool partition_contains_partition(const Partition *outer,const Partition *inner)
{
    if(inner->count>outer->count) return false;
    for(int row=0;row<inner->count;++row)
        if(inner->rows[row]>outer->rows[row]) return false;
    return true;
}

MathStatus skew_tableau_parse(const char *outer_text,const char *inner_text,
                              const char *entries_text,SkewTableau *out)
{
    SkewTableau tableau={0};
    MathStatus status=partition_parse(outer_text,&tableau.outer);
    if(status!=YT_OK) return status;
    status=partition_parse(inner_text,&tableau.inner);
    if(status!=YT_OK) return status;
    if(!partition_contains_partition(&tableau.outer,&tableau.inner)) return YT_MALFORMED;

    const char *cursor=entries_text;
    spaces(&cursor);
    if(!tableau.outer.count) {
        int values[YT_DIM],count=0;
        status=list(&cursor,values,&count,false);
        if(status!=YT_OK) return status;
        spaces(&cursor);
        if(*cursor || count) return YT_MALFORMED;
        *out=tableau;
        return YT_OK;
    }

    for(int row=0;row<tableau.outer.count;++row) {
        int values[YT_DIM],count=0;
        status=list(&cursor,values,&count,true);
        if(status!=YT_OK) return status;
        int start=partition_row(&tableau.inner,row);
        int expected=tableau.outer.rows[row]-start;
        if(count!=expected) return YT_MALFORMED;
        for(int column=0;column<count;++column)
            tableau.entries[row][start+column]=values[column];
        spaces(&cursor);
        if(row+1<tableau.outer.count) {
            if(*cursor!=';') return YT_MALFORMED;
            ++cursor;
            spaces(&cursor);
        } else if(*cursor) return YT_MALFORMED;
    }
    *out=tableau;
    return YT_OK;
}

SkewValidation skew_tableau_validate(const SkewTableau *tableau)
{
    SkewValidation validation={true,true,true,true,true,true};
    if(!partition_contains_partition(&tableau->outer,&tableau->inner))
        validation.shape=false;
    int size=partition_size(&tableau->outer)-partition_size(&tableau->inner);
    bool seen[YT_CELLS+1]={false};
    for(int row=0;row<tableau->outer.count;++row) {
        int start=partition_row(&tableau->inner,row);
        for(int column=start;column<tableau->outer.rows[row];++column) {
            int entry=tableau->entries[row][column];
            if(entry<=0) validation.positive=false;
            if(column>start && tableau->entries[row][column-1]>entry)
                validation.rows_weak=false;
            if(row>0) {
                int above_start=partition_row(&tableau->inner,row-1);
                if(column>=above_start && column<tableau->outer.rows[row-1]
                   && tableau->entries[row-1][column]>=entry)
                    validation.columns_strict=false;
            }
            if(entry<1 || entry>size || seen[entry]) validation.standard=false;
            else seen[entry]=true;
        }
    }
    for(int value=1;value<=size;++value)
        if(!seen[value]) validation.standard=false;
    validation.semistandard=validation.shape && validation.positive
        && validation.rows_weak && validation.columns_strict;
    validation.standard=validation.standard && validation.semistandard;
    return validation;
}

static bool cell_in_partition(const Partition *partition,Cell cell)
{
    return cell.row>=1 && cell.row<=partition->count
        && cell.column>=1 && cell.column<=partition->rows[cell.row-1];
}

static bool removable_cell(const Partition *partition,Cell cell)
{
    if(!cell_in_partition(partition,cell)) return false;
    int row=cell.row-1;
    return cell.column==partition->rows[row]
        && (row+1==partition->count || partition->rows[row]>partition->rows[row+1]);
}

static bool skew_cell(const SkewTableau *tableau,Cell cell)
{
    return cell_in_partition(&tableau->outer,cell)
        && !cell_in_partition(&tableau->inner,cell)
        && !(tableau->active && cell.row==tableau->hole.row
             && cell.column==tableau->hole.column);
}

bool jeu_can_begin(const SkewTableau *tableau,Cell cell)
{
    return !tableau->active && removable_cell(&tableau->inner,cell)
        && cell_in_partition(&tableau->outer,cell);
}

MathStatus jeu_begin(SkewTableau *tableau,Cell cell)
{
    if(!jeu_can_begin(tableau,cell)) return YT_MALFORMED;
    int row=cell.row-1;
    --tableau->inner.rows[row];
    if(row+1==tableau->inner.count && !tableau->inner.rows[row])
        --tableau->inner.count;
    tableau->hole=cell;
    tableau->active=true;
    return YT_OK;
}

JeuStepResult jeu_step(SkewTableau *tableau)
{
    if(!tableau->active) return JEU_INVALID;
    Cell right={tableau->hole.row,tableau->hole.column+1};
    Cell below={tableau->hole.row+1,tableau->hole.column};
    bool has_right=skew_cell(tableau,right);
    bool has_below=skew_cell(tableau,below);
    if(!has_right && !has_below) {
        if(!removable_cell(&tableau->outer,tableau->hole)) return JEU_INVALID;
        int row=tableau->hole.row-1;
        int column=tableau->hole.column-1;
        tableau->entries[row][column]=0;
        --tableau->outer.rows[row];
        if(row+1==tableau->outer.count && !tableau->outer.rows[row])
            --tableau->outer.count;
        tableau->active=false;
        return JEU_FINISHED;
    }

    Cell source;
    if(has_right && (!has_below
       || tableau->entries[right.row-1][right.column-1]
          < tableau->entries[below.row-1][below.column-1]))
        source=right;
    else
        source=below;

    int hole_row=tableau->hole.row-1,hole_column=tableau->hole.column-1;
    int source_row=source.row-1,source_column=source.column-1;
    tableau->entries[hole_row][hole_column]=tableau->entries[source_row][source_column];
    tableau->entries[source_row][source_column]=0;
    tableau->hole=source;
    return JEU_MOVED;
}

MathStatus jeu_slide(SkewTableau *tableau,Cell cell)
{
    if(tableau->active) return YT_MALFORMED;
    MathStatus status=jeu_begin(tableau,cell);
    if(status!=YT_OK) return status;
    while(tableau->active)
        if(jeu_step(tableau)==JEU_INVALID) return YT_MALFORMED;
    return YT_OK;
}

MathStatus jeu_rectify(SkewTableau *tableau)
{
    while(tableau->active)
        if(jeu_step(tableau)==JEU_INVALID) return YT_MALFORMED;
    while(tableau->inner.count) {
        int row=tableau->inner.count-1;
        Cell start={row+1,tableau->inner.rows[row]};
        MathStatus status=jeu_slide(tableau,start);
        if(status!=YT_OK) return status;
    }
    return YT_OK;
}
