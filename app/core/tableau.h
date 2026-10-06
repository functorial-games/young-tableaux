#ifndef YOUNG_TABLEAU_H
#define YOUNG_TABLEAU_H
#include "partition.h"
typedef struct { Partition shape; int entries[YT_DIM][YT_DIM]; } Tableau;
/* Kinds are checked properties of a filling, not unchecked flags on entries. */
typedef enum {
    TABLEAU_STANDARD, TABLEAU_ARBITRARY, TABLEAU_ROW_STANDARD, TABLEAU_COLUMN_STANDARD,
    TABLEAU_SEMISTANDARD, TABLEAU_SKEW, TABLEAU_SHIFTED, TABLEAU_RIBBON, TABLEAU_OSCILLATING, TABLEAU_K
} TableauKind;
MathStatus tableau_check_kind(const Tableau *tableau,TableauKind kind,bool decreasing,bool *valid);
typedef struct { bool shape, rows, columns, standard; } Validation;
typedef struct { int count; Cell cells[YT_DIM]; Cell new_cell; } InsertionTrace;
MathStatus tableau_parse(const char *text, Tableau *out);
Validation tableau_validate(const Partition *shape, const Tableau *tableau, bool decreasing);
bool tableau_semistandard(const Tableau *tableau);
MathStatus tableau_standardize(const Tableau *tableau, Tableau *out);
MathStatus tableau_row_insert(const Tableau *tableau, int value, Tableau *out, Cell *new_cell);
MathStatus tableau_row_insert_trace(const Tableau *tableau, int value, Tableau *out, InsertionTrace *trace);
MathStatus tableau_reverse_insert(const Tableau *tableau, Cell corner, Tableau *out, int *bumped_out);
MathStatus tableau_transpose(const Tableau *tableau, Tableau *out);
#endif
