#ifndef YOUNG_MATH_H
#define YOUNG_MATH_H
#include <stdbool.h>
#include <stdint.h>
#define YT_DIM 64
#define YT_CELLS 256
typedef enum { YT_OK, YT_MALFORMED, YT_LIMIT, YT_OVERFLOW } MathStatus;
typedef struct { int count; int rows[YT_DIM]; } Partition;
typedef struct { int row, column; } Cell;
typedef struct { Partition shape; int entries[YT_DIM][YT_DIM]; } Tableau;
typedef struct { bool shape, rows, columns, standard; } Validation;
const char *math_status(MathStatus status);
MathStatus partition_parse(const char *text, Partition *out);
int partition_size(const Partition *partition);
Partition partition_conjugate(const Partition *partition);
int partition_cells(const Partition *partition, Cell *out);
int partition_removable(const Partition *partition, Cell *out);
int partition_addable(const Partition *partition, Cell *out);
int partition_hook(const Partition *partition, int row, int column);
MathStatus partition_hook_product(const Partition *partition, uint64_t *out);
MathStatus partition_standard_count(const Partition *partition, uint64_t *out);
MathStatus tableau_parse(const char *text, Tableau *out);
Validation tableau_validate(const Partition *shape, const Tableau *tableau, bool decreasing);
MathStatus permutation_rsk(const char *text, Tableau *p, Tableau *q);
#endif
