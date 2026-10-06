#ifndef YOUNG_RSK_H
#define YOUNG_RSK_H
#include "tableau.h"
typedef struct { int count; int values[YT_DIM]; } Permutation;
typedef struct { int count; int letters[YT_DIM]; } Word;
typedef struct { int top, bottom; } Biletter;
typedef struct { int count; Biletter letters[YT_DIM]; } Biword;
typedef struct { int row_count, column_count; int entries[YT_DIM][YT_DIM]; } NatMatrix;
typedef struct {
    int count, step, values[YT_DIM];
    int inserted, path_count;
    Cell path[YT_DIM];
    Tableau p, q;
    bool complete;
} RSKTrace;
MathStatus permutation_parse(const char *text, Permutation *out);
MathStatus word_parse(const char *text, Word *out);
MathStatus biword_parse(const char *text, Biword *out);
MathStatus nat_matrix_parse(const char *text, NatMatrix *out);
MathStatus rsk_biword(const Biword *input,int step,RSKTrace *out);
MathStatus rsk_matrix(const NatMatrix *input,int step,RSKTrace *out);
MathStatus rsk_permutation(const Permutation *input,int step,RSKTrace *out);
MathStatus rsk_word(const Word *input,int step,RSKTrace *out);
MathStatus permutation_rsk(const char *text, Tableau *p, Tableau *q);
MathStatus word_rsk(const char *text, Tableau *p, Tableau *q);
MathStatus permutation_rsk_trace(const char *text, int step, RSKTrace *out);
MathStatus word_rsk_trace(const char *text, int step, RSKTrace *out);
#endif
