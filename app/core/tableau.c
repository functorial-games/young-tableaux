#include "tableau.h"
#include "parse.h"
#include <string.h>
MathStatus tableau_parse(const char *text, Tableau *out)
{
    Tableau tmp = {0}; const char *p = text; int size = 0;
    parse_spaces(&p);
    if (!*p || strcmp(p,"[]") == 0) { *out = tmp; return YT_OK; }
    for (;;) {
        int r = tmp.shape.count;
        if (r == YT_DIM) return YT_LIMIT;
        MathStatus s = parse_integer_list(&p,tmp.entries[r],&tmp.shape.rows[r],true);
        if (s != YT_OK) return s;
        if (!tmp.shape.rows[r]) {
            parse_spaces(&p);
            if (!r && !*p) { *out = tmp; return YT_OK; }
            return YT_MALFORMED;
        }
        size += tmp.shape.rows[r];
        if (size > YT_CELLS) return YT_LIMIT;
        ++tmp.shape.count; parse_spaces(&p);
        if (!*p) break;
        if (*p++ != ';') return YT_MALFORMED;
        parse_spaces(&p); if (!*p) return YT_MALFORMED;
    }
    *out = tmp; return YT_OK;
}
static bool tableau_shape_valid(const Partition *shape)
{
    if(shape->count<0 || shape->count>YT_DIM) return false;
    int size=0;
    for(int row=0;row<shape->count;++row) {
        if(shape->rows[row]<=0 || shape->rows[row]>YT_DIM) return false;
        if(row && shape->rows[row]>shape->rows[row-1]) return false;
        if(size>YT_CELLS-shape->rows[row]) return false;
        size+=shape->rows[row];
    }
    return true;
}

static bool same_shape(const Partition *first,const Partition *second)
{
    if(first->count!=second->count) return false;
    for(int row=0;row<first->count;++row) if(first->rows[row]!=second->rows[row]) return false;
    return true;
}

static bool rows_ordered(const Tableau *tableau,bool decreasing,bool weak)
{
    for(int row=0;row<tableau->shape.count;++row) for(int column=1;column<tableau->shape.rows[row];++column) {
        int before=tableau->entries[row][column-1],after=tableau->entries[row][column];
        if(decreasing ? (weak?before<after:before<=after) : (weak?before>after:before>=after)) return false;
    }
    return true;
}

static bool columns_ordered(const Tableau *tableau,bool decreasing)
{
    for(int row=1;row<tableau->shape.count;++row) for(int column=0;column<tableau->shape.rows[row];++column) {
        int before=tableau->entries[row-1][column],after=tableau->entries[row][column];
        if(decreasing?before<=after:before>=after) return false;
    }
    return true;
}

static bool standard_alphabet(const Tableau *tableau)
{
    int size=partition_size(&tableau->shape);
    bool seen[YT_CELLS+1]={false};
    for(int row=0;row<tableau->shape.count;++row) for(int column=0;column<tableau->shape.rows[row];++column) {
        int value=tableau->entries[row][column];
        if(value<1 || value>size || seen[value]) return false;
        seen[value]=true;
    }
    return true;
}

Validation tableau_validate(const Partition *shape,const Tableau *tableau,bool decreasing)
{
    Validation result={0};
    if(!shape || !tableau || !tableau_shape_valid(shape) || !tableau_shape_valid(&tableau->shape)) return result;
    result.shape=same_shape(shape,&tableau->shape);
    result.rows=rows_ordered(tableau,decreasing,false);
    result.columns=columns_ordered(tableau,decreasing);
    result.standard=result.shape && result.rows && result.columns && standard_alphabet(tableau);
    return result;
}

bool tableau_semistandard(const Tableau *tableau)
{
    return tableau && tableau_shape_valid(&tableau->shape) && rows_ordered(tableau,false,true) && columns_ordered(tableau,false);
}

MathStatus tableau_check_kind(const Tableau *tableau,TableauKind kind,bool decreasing,bool *valid)
{
    if(!tableau || !valid || !tableau_shape_valid(&tableau->shape)) return YT_MALFORMED;
    switch(kind) {
    case TABLEAU_ARBITRARY: *valid=true; break;
    case TABLEAU_ROW_STANDARD: *valid=standard_alphabet(tableau) && rows_ordered(tableau,decreasing,false); break;
    case TABLEAU_COLUMN_STANDARD: *valid=standard_alphabet(tableau) && columns_ordered(tableau,decreasing); break;
    case TABLEAU_STANDARD: *valid=tableau_validate(&tableau->shape,tableau,decreasing).standard; break;
    case TABLEAU_SEMISTANDARD: *valid=rows_ordered(tableau,decreasing,true) && columns_ordered(tableau,decreasing); break;
    case TABLEAU_SKEW: case TABLEAU_SHIFTED: case TABLEAU_RIBBON: case TABLEAU_OSCILLATING: case TABLEAU_K:
        return YT_UNSUPPORTED;
    default: return YT_MALFORMED;
    }
    return YT_OK;
}

typedef struct {
    int value,row,column;
} TableauRankItem;

static bool rank_after(TableauRankItem left,TableauRankItem right)
{
    if(left.value!=right.value) return left.value>right.value;
    if(left.column!=right.column) return left.column>right.column;
    return left.row>right.row;
}

MathStatus tableau_standardize(const Tableau *tableau,Tableau *out)
{
    if(!tableau || !out || !tableau_semistandard(tableau)) return YT_MALFORMED;
    Tableau result=*tableau;
    TableauRankItem items[YT_CELLS];
    int count=0;
    for(int row=0;row<tableau->shape.count;++row)
        for(int column=0;column<tableau->shape.rows[row];++column)
            items[count++]=(TableauRankItem){tableau->entries[row][column],row,column};

    for(int index=1;index<count;++index) {
        TableauRankItem item=items[index];
        int position=index;
        while(position>0 && rank_after(items[position-1],item)) {
            items[position]=items[position-1];
            --position;
        }
        items[position]=item;
    }
    for(int index=0;index<count;++index)
        result.entries[items[index].row][items[index].column]=index+1;

    Validation validation=tableau_validate(&result.shape,&result,false);
    if(!validation.standard) return YT_MALFORMED;
    *out=result;
    return YT_OK;
}

static int first_larger_column(const Tableau *tableau,int row,int value)
{
    int column=0;
    while(column<tableau->shape.rows[row] && tableau->entries[row][column]<=value) ++column;
    return column;
}

static int bump_row_entry(Tableau *tableau,int row,int column,int value)
{
    int bumped=tableau->entries[row][column];
    tableau->entries[row][column]=value;
    return bumped;
}

static MathStatus grow_insertion_shape(Tableau *tableau,int row,int column,int value)
{
    if(column>=YT_DIM) return YT_LIMIT;
    Partition grown;
    MathStatus status=partition_add_cell(&tableau->shape,(Cell){row+1,column+1},&grown);
    if(status!=YT_OK) return status;
    tableau->shape=grown;
    tableau->entries[row][column]=value;
    return YT_OK;
}

MathStatus tableau_row_insert_trace(const Tableau *tableau,int value,Tableau *out,InsertionTrace *trace)
{
    if(!tableau || !out || !tableau_semistandard(tableau)) return YT_MALFORMED;
    if(partition_size(&tableau->shape)>=YT_CELLS) return YT_LIMIT;
    Tableau result=*tableau;
    InsertionTrace path={0};
    int bumped=value;
    for(int row=0;row<YT_DIM;++row) {
        int length=row<result.shape.count?result.shape.rows[row]:0;
        int column=row<result.shape.count?first_larger_column(&result,row,bumped):0;
        path.cells[path.count++]=(Cell){row+1,column+1};
        if(column==length) {
            MathStatus status=grow_insertion_shape(&result,row,column,bumped);
            if(status!=YT_OK) return status;
            path.new_cell=(Cell){row+1,column+1};
            if(!tableau_semistandard(&result)) return YT_MALFORMED;
            *out=result;
            if(trace) *trace=path;
            return YT_OK;
        }
        bumped=bump_row_entry(&result,row,column,bumped);
    }
    return YT_LIMIT;
}

MathStatus tableau_row_insert(const Tableau *tableau,int value,Tableau *out,Cell *new_cell)
{
    InsertionTrace trace;
    MathStatus status=tableau_row_insert_trace(tableau,value,out,&trace);
    if(status==YT_OK && new_cell) *new_cell=trace.new_cell;
    return status;
}

static bool tableau_removable_corner(const Tableau *tableau,Cell corner)
{
    if(corner.row<1 || corner.row>tableau->shape.count) return false;
    int row=corner.row-1;
    if(corner.column!=tableau->shape.rows[row]) return false;
    return row+1==tableau->shape.count
        || tableau->shape.rows[row]>tableau->shape.rows[row+1];
}

MathStatus tableau_reverse_insert(const Tableau *tableau,Cell corner,
                                  Tableau *out,int *bumped_out)
{
    if(!tableau || !out || !tableau_semistandard(tableau)
       || !tableau_removable_corner(tableau,corner)) return YT_MALFORMED;
    Tableau result=*tableau;
    int row=corner.row-1,column=corner.column-1;
    int bumped=result.entries[row][column];
    --result.shape.rows[row];
    if(!result.shape.rows[row]) --result.shape.count;

    for(int upper=row-1;upper>=0;--upper) {
        int candidate=result.shape.rows[upper]-1;
        while(candidate>=0 && result.entries[upper][candidate]>=bumped) --candidate;
        if(candidate<0) return YT_MALFORMED;
        int next=result.entries[upper][candidate];
        result.entries[upper][candidate]=bumped;
        bumped=next;
    }
    if(!tableau_semistandard(&result)) return YT_MALFORMED;
    if(bumped_out) *bumped_out=bumped;
    *out=result;
    return YT_OK;
}

MathStatus tableau_transpose(const Tableau *tableau,Tableau *out)
{
    if(!tableau || !out || !tableau_shape_valid(&tableau->shape)) return YT_MALFORMED;
    Tableau result={0};
    result.shape=partition_conjugate(&tableau->shape);
    for(int row=0;row<tableau->shape.count;++row)
        for(int column=0;column<tableau->shape.rows[row];++column)
            result.entries[column][row]=tableau->entries[row][column];
    *out=result;
    return YT_OK;
}
