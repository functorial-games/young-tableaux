#ifndef YOUNG_DIAGRAM_H
#define YOUNG_DIAGRAM_H
#include "partition.h"
typedef struct {
    Partition base, current;
    int addition_count;
    Cell additions[YT_CELLS];
} DiagramState;
void diagram_replace(DiagramState *state, const Partition *partition);
MathStatus diagram_add_cell(DiagramState *state, Cell cell);
MathStatus diagram_remove_cell(DiagramState *state, Cell cell);
MathStatus diagram_undo_addition(DiagramState *state);
void diagram_reset_additions(DiagramState *state);
#endif
