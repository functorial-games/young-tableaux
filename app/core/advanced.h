#ifndef YOUNG_ADVANCED_H
#define YOUNG_ADVANCED_H
#include "young_math.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define YT_RESULT_PARTITIONS 256
#define YT_RESULT_TABLEAUX 32
#define YT_RESULT_PATHS 64
#define YT_RESULT_TERMS 128
#define YT_ADVANCED_DEGREE_MAX 16
#define YT_PLETHYSM_DEGREE_MAX 10
#define YT_PATH_LENGTH_MAX 17

typedef struct { int count; int values[YT_CELLS]; } NumberList;
typedef struct { int count; Partition values[YT_RESULT_PARTITIONS]; } PartitionList;
typedef struct { int count; SkewTableau values[YT_RESULT_TABLEAUX]; } SkewTableauList;
typedef struct { int count; Partition values[YT_PATH_LENGTH_MAX]; } PartitionPath;
typedef struct { int count; PartitionPath values[YT_RESULT_PATHS]; } PartitionPathList;

MathStatus biword_rsk(const char *text, Tableau *p, Tableau *q);
MathStatus matrix_rsk(const char *text, Tableau *p, Tableau *q);
MathStatus inverse_rsk(const Tableau *p, const Tableau *q, NumberList *permutation);
MathStatus tableau_promote(const Tableau *tableau, Tableau *out);
MathStatus tableau_evacuate(const Tableau *tableau, Tableau *out);

MathStatus littlewood_richardson_coefficient(const Partition *inner,
                                              const Partition *content,
                                              const Partition *outer,
                                              uint64_t *out);
MathStatus littlewood_richardson_tableaux(const Partition *inner,
                                           const Partition *content,
                                           const Partition *outer,
                                           SkewTableauList *out);
MathStatus schur_product_text(const Partition *left,const Partition *right,
                              char *out,size_t out_size);

MathStatus permutation_cycle_type(const char *text,Partition *cycle_type);
MathStatus symmetric_character_cycle_type(const Partition *shape,
                                           const Partition *cycle_type,
                                           int64_t *out);
MathStatus symmetric_character_permutation(const Partition *shape,
                                            const char *permutation,
                                            int64_t *out);

MathStatus schur_change_basis_text(const Partition *shape,const char *basis,
                                   const char *q,const char *t,
                                   char *out,size_t out_size);
MathStatus schur_specialization_text(const Partition *shape,const char *variables,
                                     char *out,size_t out_size);
MathStatus schur_plethysm_text(const Partition *outer,const Partition *inner,
                               char *out,size_t out_size);

MathStatus partition_branch_up(const Partition *partition,PartitionList *out);
MathStatus partition_branch_down(const Partition *partition,PartitionList *out);
MathStatus young_graph_paths(const Partition *start,const Partition *end,
                             PartitionPathList *out);

uint64_t young_rng_next(uint64_t *state);
MathStatus random_permutation(int n,uint64_t *state,NumberList *out);
MathStatus random_standard_tableau(const Partition *shape,uint64_t *state,Tableau *out);
MathStatus sample_plancherel_partition(int n,uint64_t *state,Partition *out);

MathStatus permutation_lis(const char *text,NumberList *out);
MathStatus permutation_lds(const char *text,NumberList *out);
MathStatus permutation_coxeter_reduced_word(const char *text,NumberList *out);
MathStatus permutation_bruhat_leq(const char *left,const char *right,bool *out);

#endif
