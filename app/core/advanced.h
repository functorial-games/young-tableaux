#ifndef YOUNG_ADVANCED_H
#define YOUNG_ADVANCED_H
#include "young_math.h"
#include <stddef.h>
/* Exact bounded arithmetic. A limit is an error, never a truncated answer. */
#define YT_ADVANCED_DEGREE_MAX 8
#define YT_RESULT_PARTITIONS 32
#define YT_RESULT_TABLEAUX 32
#define YT_RESULT_PATHS 128
#define YT_RESULT_TERMS 32
#define YT_PATH_LENGTH_MAX 17
typedef struct { int count; int values[YT_CELLS]; } NumberList;
typedef struct { int count; Partition values[YT_RESULT_PARTITIONS]; } PartitionList;
typedef struct { int count; SkewTableau values[YT_RESULT_TABLEAUX]; } SkewTableauList;
typedef struct { int count; Partition values[YT_PATH_LENGTH_MAX]; } PartitionPath;
typedef struct { int count; PartitionPath values[YT_RESULT_PATHS]; } PartitionPathList;
typedef struct { int64_t numerator; uint64_t denominator; } Rational;
typedef enum { BASIS_SCHUR, BASIS_POWER, BASIS_MONOMIAL, BASIS_COMPLETE, BASIS_ELEMENTARY } SymmetricBasis;
typedef struct { Partition index; Rational coefficient; } SymmetricTerm;
typedef struct { int count; SymmetricTerm terms[YT_RESULT_TERMS]; SymmetricBasis basis; } SymmetricFunction;
typedef struct { uint64_t state; } YoungRNG;
MathStatus inverse_rsk(const PermutationRSKResult *input,Permutation *out);
MathStatus tableau_promote(const StandardTableau *input,StandardTableau *out);
MathStatus tableau_evacuate(const StandardTableau *input,StandardTableau *out);
MathStatus littlewood_richardson_coefficient(const Partition *inner,const Partition *content,const Partition *outer,uint64_t *out);
MathStatus littlewood_richardson_tableaux(const Partition *inner,const Partition *content,const Partition *outer,SkewTableauList *out);
MathStatus symmetric_character_cycle_type(const Partition *shape,const Partition *cycle_type,int64_t *out);
MathStatus permutation_cycle_type(const Permutation *input,Partition *out);
MathStatus symmetric_schur_product(const Partition *left,const Partition *right,SymmetricFunction *out);
MathStatus symmetric_change_basis(const SymmetricFunction *input,SymmetricBasis basis,SymmetricFunction *out);
MathStatus symmetric_plethysm(const SymmetricFunction *outer,const SymmetricFunction *inner,SymmetricFunction *out);
MathStatus symmetric_specialize(const SymmetricFunction *input,const int *alphabet,int count,Rational *out);
MathStatus symmetric_parse(const char *text,SymmetricFunction *out);
MathStatus symmetric_text(const SymmetricFunction *input,char *out,size_t capacity);
MathStatus partition_branch_up(const Partition *input,PartitionList *out);
MathStatus partition_branch_down(const Partition *input,PartitionList *out);
MathStatus young_graph_paths(const Partition *start,const Partition *end,PartitionPathList *out);
uint64_t young_rng_next(YoungRNG *rng);
MathStatus random_permutation(int count,YoungRNG *rng,Permutation *out);
MathStatus random_standard_tableau(const Partition *shape,YoungRNG *rng,StandardTableau *out);
MathStatus sample_plancherel_partition(int count,YoungRNG *rng,Partition *out);
MathStatus permutation_lis(const Permutation *input,NumberList *out);
MathStatus permutation_lds(const Permutation *input,NumberList *out);
MathStatus permutation_coxeter_reduced_word(const Permutation *input,NumberList *out);
MathStatus permutation_bruhat_leq(const Permutation *left,const Permutation *right,bool *out);
#endif
