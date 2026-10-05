# Computational references

These links are for checking examples, generating data, and comparing eventual implementations against mature systems.

## SageMath

- **Tableaux**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/tableau.html
  - Standard/semistandard tableaux, shapes, insertion-related operations and many tableau methods.

- **Partitions and related combinatorics catalog**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/catalog_partitions.html

- **Permutations**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/permutation.html

- **Robinson–Schensted–Knuth correspondence**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/rsk.html

- **Symmetric functions**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/sf/sf.html

- **Symmetric-function algebra machinery**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/sf/sfa.html

- **Representations of the symmetric group**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/symmetric_group_representations.html

- **Symmetric group algebra**
  - https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/symmetric_group_algebra.html

- **Growth diagrams**
  - Search the Sage combinatorics docs around GrowthDiagram and RuleRSK; Sage implements Fomin-style local rules and multiple correspondence variants.
  - Source/tutorial entry point: https://sporadic.stanford.edu/thematic_tutorials/algebraic_combinatorics/rsk.html

- **Classical crystals**
  - https://doc.sagemath.org/html/en/thematic_tutorials/lie/crystals.html
  - Tableau models for crystal bases and their representation-theoretic meaning.

## GAP

- **Character Tables**
  - https://docs.gap-system.org/doc/ref/chap71.html

- **Class Functions**
  - https://docs.gap-system.org/doc/ref/chap72.html

- **Character Table Library tutorial**
  - https://docs.gap-system.org/pkg/ctbllib/doc/chap2.html

- **Hecke / Specht functionality**
  - https://gap-packages.github.io/hecke/doc/chap3.html
  - Computational Specht-module and Hecke-algebra material.

- **Hecke package contents**
  - https://gap-packages.github.io/hecke/doc/chap0_mj.html

## What to use these for

Candidate regression checks for later code:

- enumerate partitions of n;
- enumerate standard tableaux of a fixed shape;
- verify hook-length counts;
- compute conjugate partitions, corners and contents;
- run RSK in both directions;
- compare row/column insertion conventions;
- compute Schur functions and products;
- extract Littlewood–Richardson coefficients;
- build irreducible symmetric-group representations / characters;
- test branching S_n -> S_(n-1) and induction in the other direction;
- sample or enumerate permutations by RSK shape;
- compare local growth-diagram implementations with a mature implementation.

The repository should not depend on Sage or GAP merely because they are listed here. They are excellent oracles for small examples and test-vector generation.
