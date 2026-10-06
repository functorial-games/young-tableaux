#ifndef YOUNG_JEU_DE_TAQUIN_H
#define YOUNG_JEU_DE_TAQUIN_H
#include "tableau.h"
typedef struct { Partition outer, inner; } SkewShape;
typedef struct { SkewShape shape; int entries[YT_DIM][YT_DIM]; } SkewTableau;
/* A slide owns a hole; a completed filling never has a hole field. */
typedef struct { SkewTableau filling; Cell hole; bool active; } JeuState;
typedef struct {
    bool shape, rows_weak, columns_strict, positive, semistandard, standard;
} SkewValidation;
typedef enum { JEU_INVALID, JEU_MOVED, JEU_FINISHED } JeuStepResult;
MathStatus jeu_state_parse(const char *outer,const char *inner,const char *entries,JeuState *out);
MathStatus skew_tableau_parse(const char *outer, const char *inner, const char *entries, SkewTableau *out);
SkewValidation skew_tableau_validate(const SkewTableau *tableau);
bool jeu_can_begin(const JeuState *tableau, Cell cell);
MathStatus jeu_begin(JeuState *tableau, Cell cell);
JeuStepResult jeu_step(JeuState *tableau);
MathStatus jeu_slide(JeuState *tableau, Cell cell);
MathStatus jeu_rectify(JeuState *tableau);
#endif
