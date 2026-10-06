#include "diagram.h"
void diagram_replace(DiagramState *state, const Partition *partition)
{
    *state=(DiagramState){.base=*partition,.current=*partition};
}
MathStatus diagram_add_cell(DiagramState *state, Cell cell)
{
    if(state->addition_count>=YT_CELLS) return YT_LIMIT;
    Partition grown;
    MathStatus status=partition_add_cell(&state->current,cell,&grown);
    if(status!=YT_OK) return status;
    state->current=grown;
    state->additions[state->addition_count++]=cell;
    return YT_OK;
}
MathStatus diagram_remove_cell(DiagramState *state, Cell cell)
{
    Partition reduced;
    MathStatus status=partition_remove_cell(&state->current,cell,&reduced);
    if(status==YT_OK) diagram_replace(state,&reduced);
    return status;
}
MathStatus diagram_undo_addition(DiagramState *state)
{
    if(state->addition_count<=0) return YT_MALFORMED;
    Partition reduced;
    MathStatus status=partition_remove_cell(&state->current,state->additions[state->addition_count-1],&reduced);
    if(status!=YT_OK) return status;
    state->current=reduced;
    --state->addition_count;
    return YT_OK;
}
void diagram_reset_additions(DiagramState *state)
{
    state->current=state->base;
    state->addition_count=0;
}
