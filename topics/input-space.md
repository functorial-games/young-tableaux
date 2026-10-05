# Mathematical input space

This file inventories mathematical inputs without committing to a user-interface design.

## Basic combinatorial objects

### Integer / size

- n = number of boxes, size of a partition, size of a permutation, or rank of S_n depending on context.

### Partition / Young diagram

- lambda = (lambda_1, lambda_2, ...)
- conjugate partition lambda'
- optional inner partition mu subseteq lambda for a skew shape lambda/mu

Once lambda is fixed, the following are outputs rather than independent inputs:
- hook lengths;
- arm/leg lengths;
- corners / addable and removable boxes;
- contents j-i (up to convention);
- hook product;
- number f^lambda of standard Young tableaux;
- the characteristic-zero irreducible S_n representation indexed by lambda.

### Filling / tableau

Possible input distinctions:
- arbitrary filling;
- row-standard;
- column-standard;
- standard Young tableau;
- semistandard Young tableau;
- skew tableau;
- shifted / oscillating / ribbon / k-tableau in later extensions.

For semistandard tableaux also record:
- alphabet / maximum letter;
- content or weight mu = (mu_1, mu_2, ...), where mu_i counts entries equal to i.

### Equivalent encodings of a standard tableau

A standard Young tableau can be encoded by:
- the filled diagram itself;
- the order in which boxes were added;
- a saturated path in Young's lattice from the empty partition to lambda.

These are alternative encodings of essentially the same mathematical input, not three independent knobs.

## RSK-family inputs

Classical inputs include:
- a permutation in S_n;
- a word over an ordered alphabet;
- a biword;
- a nonnegative-integer matrix.

Typical output:
- a pair (P,Q) of tableaux of the same shape, with the precise tableau classes depending on the input and convention.

Potential additional inputs:
- row insertion vs column insertion;
- ordering / tie convention;
- forward vs inverse correspondence.

Again, these conventions should be recorded separately from the underlying data.

## Local tableau operations

Inputs may include:
- a selected cell;
- an addable corner;
- a removable corner;
- an inserted letter;
- an inner/outer corner for jeu de taquin;
- a chosen local move;
- a direction / convention where the literature has dual versions.

Operations worth keeping distinct:
- row insertion;
- column insertion;
- reverse insertion;
- jeu de taquin slide;
- rectification;
- promotion;
- evacuation;
- conjugation / transpose.

## Littlewood–Richardson / Schur-function inputs

Natural independent data:
- two partitions lambda and mu for a product s_lambda s_mu;
- a third partition nu when asking for a specific coefficient c^nu_(lambda,mu);
- a skew shape nu/lambda;
- content mu for LR-tableau formulations.

Possible outputs:
- LR tableaux;
- Littlewood–Richardson coefficient;
- Schur expansion.

## Symmetric-group representation inputs

At the characteristic-zero level:
- n;
- partition lambda of n;
- group element / conjugacy class, often represented by a permutation or cycle type;
- optional subgroup/branching step S_(n-1) subset S_n.

Potential later parameters:
- base field;
- characteristic p for modular representation theory;
- Hecke parameter q for q-deformations;
- tableau/basis convention.

The characteristic-p and Hecke cases substantially change the theory and should not be silently folded into the ordinary characteristic-zero story.

## Symmetric functions

Possible independent inputs:
- basis: monomial, elementary, complete homogeneous, power-sum, Schur, Hall–Littlewood, Macdonald, etc.;
- partition indexing a basis element;
- coefficients / linear combination;
- variables or specialization;
- parameters q,t where applicable;
- multiplication, coproduct, scalar product, plethysm, specialization or basis change as the chosen operation.

## Random / asymptotic experiments

Possible probability-law inputs:
- uniform random permutation in S_n, followed by RSK;
- Plancherel measure on partitions of n;
- uniform standard tableau of fixed shape;
- other measures on partitions/tableaux.

Then n, the law, and any conditioning are genuine inputs; limit shape, LIS length, empirical row lengths, etc. are outputs.

## Young graph / branching inputs

Useful graph-level data:
- starting partition;
- ending partition;
- direction: induction/add a box or restriction/remove a box;
- path, if a specific branching history is supplied.

A path in the Young graph is again equivalent to a standard tableau when it begins at the empty partition and adds one box at a time.

## Coxeter / type-A bridge

Because type A Coxeter groups are symmetric groups, additional encodings may become relevant:
- simple-transposition word;
- reduced word;
- permutation;
- Bruhat or weak-order interval;
- Coxeter element / parabolic subgroup.

These are particularly relevant to cross-links with the Coxeter repository, but they need not be part of the first tableaux implementation.

## Conventions to record, not confuse with mathematics

- English vs French Young-diagram orientation;
- row/column indexing origin;
- content convention j-i vs i-j;
- row vs column insertion;
- reading-word convention;
- left vs right group action;
- multiplication order for permutations.

Many apparent disagreements in examples are convention mismatches. Any executable code should state them explicitly.
