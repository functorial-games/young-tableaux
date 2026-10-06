#ifndef YOUNG_PARTITION_H
#define YOUNG_PARTITION_H
#include "math_types.h"
typedef struct { int count; int rows[YT_DIM]; } Partition;
typedef struct { int row, column; } Cell;
MathStatus partition_parse(const char *text, Partition *out);
int partition_size(const Partition *partition);
Partition partition_conjugate(const Partition *partition);
int partition_cells(const Partition *partition, Cell *out);
int partition_removable(const Partition *partition, Cell *out);
int partition_addable(const Partition *partition, Cell *out);
int partition_hook(const Partition *partition, int row, int column);
MathStatus partition_hook_product(const Partition *partition, uint64_t *out);
MathStatus partition_standard_count(const Partition *partition, uint64_t *out);
MathStatus partition_add_cell(const Partition *partition, Cell cell, Partition *out);
MathStatus partition_remove_cell(const Partition *partition, Cell cell, Partition *out);
#endif
