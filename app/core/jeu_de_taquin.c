#include "jeu_de_taquin.h"
#include "parse.h"
#include <string.h>
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
    MathStatus status=partition_parse(outer_text,&tableau.shape.outer);
    if(status!=YT_OK) return status;
    status=partition_parse(inner_text,&tableau.shape.inner);
    if(status!=YT_OK) return status;
    if(!partition_contains_partition(&tableau.shape.outer,&tableau.shape.inner)) return YT_MALFORMED;

    const char *cursor=entries_text;
    parse_spaces(&cursor);
    if(!tableau.shape.outer.count) {
        int values[YT_DIM],count=0;
        status=parse_integer_list(&cursor,values,&count,false);
        if(status!=YT_OK) return status;
        parse_spaces(&cursor);
        if(*cursor || count) return YT_MALFORMED;
        *out=tableau;
        return YT_OK;
    }

    for(int row=0;row<tableau.shape.outer.count;++row) {
        int values[YT_DIM],count=0;
        status=parse_integer_list(&cursor,values,&count,true);
        if(status!=YT_OK) return status;
        int start=partition_row(&tableau.shape.inner,row);
        int expected=tableau.shape.outer.rows[row]-start;
        if(count!=expected) return YT_MALFORMED;
        for(int column=0;column<count;++column)
            tableau.entries[row][start+column]=values[column];
        parse_spaces(&cursor);
        if(row+1<tableau.shape.outer.count) {
            if(*cursor!=';') return YT_MALFORMED;
            ++cursor;
            parse_spaces(&cursor);
        } else if(*cursor) return YT_MALFORMED;
    }
    *out=tableau;
    return YT_OK;
}

SkewValidation skew_tableau_validate(const SkewTableau *tableau)
{
    SkewValidation validation={true,true,true,true,true,true};
    if(!partition_contains_partition(&tableau->shape.outer,&tableau->shape.inner))
        validation.shape=false;
    int size=partition_size(&tableau->shape.outer)-partition_size(&tableau->shape.inner);
    bool seen[YT_CELLS+1]={false};
    for(int row=0;row<tableau->shape.outer.count;++row) {
        int start=partition_row(&tableau->shape.inner,row);
        for(int column=start;column<tableau->shape.outer.rows[row];++column) {
            int entry=tableau->entries[row][column];
            if(entry<=0) validation.positive=false;
            if(column>start && tableau->entries[row][column-1]>entry)
                validation.rows_weak=false;
            if(row>0) {
                int above_start=partition_row(&tableau->shape.inner,row-1);
                if(column>=above_start && column<tableau->shape.outer.rows[row-1]
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

MathStatus jeu_state_parse(const char *outer,const char *inner,const char *entries,JeuState *out)
{
    if(!out) return YT_MALFORMED;
    SkewTableau filling;
    MathStatus status=skew_tableau_parse(outer,inner,entries,&filling);
    if(status==YT_OK) *out=(JeuState){.filling=filling};
    return status;
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

static bool skew_cell(const JeuState *tableau,Cell cell)
{
    return cell_in_partition(&tableau->filling.shape.outer,cell)
        && !cell_in_partition(&tableau->filling.shape.inner,cell)
        && !(tableau->active && cell.row==tableau->hole.row
             && cell.column==tableau->hole.column);
}

bool jeu_can_begin(const JeuState *tableau,Cell cell)
{
    return !tableau->active && removable_cell(&tableau->filling.shape.inner,cell)
        && cell_in_partition(&tableau->filling.shape.outer,cell);
}

MathStatus jeu_begin(JeuState *tableau,Cell cell)
{
    if(!jeu_can_begin(tableau,cell)) return YT_MALFORMED;
    Partition smaller;
    MathStatus status=partition_remove_cell(&tableau->filling.shape.inner,cell,&smaller);
    if(status!=YT_OK) return status;
    tableau->filling.shape.inner=smaller;
    tableau->hole=cell;
    tableau->active=true;
    return YT_OK;
}

static JeuStepResult finish_slide_at_outer_corner(JeuState *tableau)
{
    Partition smaller;
    MathStatus status=partition_remove_cell(&tableau->filling.shape.outer,tableau->hole,&smaller);
    if(status!=YT_OK) return JEU_INVALID;
    tableau->filling.entries[tableau->hole.row-1][tableau->hole.column-1]=0;
    tableau->filling.shape.outer=smaller;
    tableau->active=false;
    return JEU_FINISHED;
}

static Cell smaller_slide_neighbor(const JeuState *tableau,Cell right,Cell below,
                                   bool has_right,bool has_below)
{
    /* Equal entries slide up from below: rows weak, columns strict. */
    if(has_right && (!has_below || tableau->filling.entries[right.row-1][right.column-1]
                      < tableau->filling.entries[below.row-1][below.column-1])) return right;
    return below;
}

static void move_entry_into_hole(JeuState *tableau,Cell source)
{
    tableau->filling.entries[tableau->hole.row-1][tableau->hole.column-1]=tableau->filling.entries[source.row-1][source.column-1];
    tableau->filling.entries[source.row-1][source.column-1]=0;
    tableau->hole=source;
}

JeuStepResult jeu_step(JeuState *tableau)
{
    if(!tableau->active) return JEU_INVALID;
    Cell right={tableau->hole.row,tableau->hole.column+1};
    Cell below={tableau->hole.row+1,tableau->hole.column};
    bool has_right=skew_cell(tableau,right);
    bool has_below=skew_cell(tableau,below);
    if(!has_right && !has_below) return finish_slide_at_outer_corner(tableau);
    Cell source=smaller_slide_neighbor(tableau,right,below,has_right,has_below);
    move_entry_into_hole(tableau,source);
    return JEU_MOVED;
}

MathStatus jeu_slide(JeuState *tableau,Cell cell)
{
    if(tableau->active) return YT_MALFORMED;
    MathStatus status=jeu_begin(tableau,cell);
    if(status!=YT_OK) return status;
    while(tableau->active)
        if(jeu_step(tableau)==JEU_INVALID) return YT_MALFORMED;
    return YT_OK;
}

MathStatus jeu_rectify(JeuState *tableau)
{
    while(tableau->active)
        if(jeu_step(tableau)==JEU_INVALID) return YT_MALFORMED;
    while(tableau->filling.shape.inner.count) {
        int row=tableau->filling.shape.inner.count-1;
        Cell start={row+1,tableau->filling.shape.inner.rows[row]};
        MathStatus status=jeu_slide(tableau,start);
        if(status!=YT_OK) return status;
    }
    return YT_OK;
}
