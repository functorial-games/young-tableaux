#ifndef YOUNG_MATH_H
#define YOUNG_MATH_H
#include <stdbool.h>
#include <stdint.h>
#define YT_DIM 64
#define YT_CELLS 256
typedef enum { YT_OK, YT_MALFORMED, YT_LIMIT, YT_OVERFLOW, YT_COMPLEXITY } MathStatus;
typedef struct { int count; int rows[YT_DIM]; } Partition;
typedef struct { int row, column; } Cell;
typedef struct { Partition shape; int entries[YT_DIM][YT_DIM]; } Tableau;
typedef struct { bool shape, rows, columns, standard; } Validation;
typedef struct {
    int count, step, values[YT_DIM];
    int inserted, path_count;
    Cell path[YT_DIM];
    Tableau p, q;
    bool complete;
} RSKTrace;
typedef struct {
    Partition outer, inner;
    int entries[YT_DIM][YT_DIM];
    Cell hole;
    bool active;
} SkewTableau;
typedef struct {
    bool shape, rows_weak, columns_strict, positive, semistandard, standard;
} SkewValidation;
typedef enum { JEU_INVALID, JEU_MOVED, JEU_FINISHED } JeuStepResult;
const char *math_status(MathStatus status);
/* Status-returning entry points reject malformed input and leave outputs
 * untouched on failure. Raw selectors below require a validated partition.
 * partition_cells needs 256 Cell slots; addable needs 65; removable needs 64.
 * Mathematical addable corners may exceed the bounded storage dimensions;
 * partition_add_cell reports YT_LIMIT rather than creating an invalid value. */
MathStatus partition_validate(const Partition *partition);
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
/* Ordinary insertion permits ordered signed integer entries. Word RSK and
 * skew jeu de taquin use the positive-integer alphabet declared in the app.
 * Transposition of a semistandard filling need not remain semistandard. */
bool tableau_semistandard(const Tableau *tableau);
MathStatus tableau_standardize(const Tableau *tableau, Tableau *out);
MathStatus tableau_row_insert(const Tableau *tableau, int value, Tableau *out, Cell *new_cell);
MathStatus tableau_reverse_insert(const Tableau *tableau, Cell corner, Tableau *out, int *bumped_out);
MathStatus tableau_transpose(const Tableau *tableau, Tableau *out);
MathStatus partition_add_cell(const Partition *partition, Cell cell, Partition *out);
MathStatus partition_remove_cell(const Partition *partition, Cell cell, Partition *out);
/* P and Q must be distinct output objects; errors do not overwrite them. */
MathStatus permutation_rsk(const char *text, Tableau *p, Tableau *q);
MathStatus word_rsk(const char *text, Tableau *p, Tableau *q);
MathStatus permutation_rsk_trace(const char *text, int step, RSKTrace *out);
MathStatus word_rsk_trace(const char *text, int step, RSKTrace *out);
MathStatus skew_tableau_parse(const char *outer, const char *inner, const char *entries, SkewTableau *out);
/* A live slide has a hole and is not validated as a completed tableau.
 * begin/step/slide/rectify each validate their own supported state. */
SkewValidation skew_tableau_validate(const SkewTableau *tableau);
bool jeu_can_begin(const SkewTableau *tableau, Cell cell);
MathStatus jeu_begin(SkewTableau *tableau, Cell cell);
JeuStepResult jeu_step(SkewTableau *tableau);
MathStatus jeu_slide(SkewTableau *tableau, Cell cell);
MathStatus jeu_rectify(SkewTableau *tableau);
#endif
