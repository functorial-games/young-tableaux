# Consolidated ordinary Young Tableaux surface

The operation registry is executable. `combinatorics.c` owns inverse permutation
RSK, standard-tableau promotion/evacuation, Young-graph neighbors and paths,
random generation, subsequences, reduced words, and strong Bruhat order.
`algebra.c` owns LR enumeration, ordinary complex characters, and exact finite
symmetric functions. Existing partition, tableau, insertion, and jeu owners
remain underneath these modules.

## Mathematical contracts

- Inverse RSK accepts checked standard P and Q of equal shape and returns a
  permutation. It does not pretend to invert the word or matrix result family.
- Promotion deletes 1, follows the existing forward jeu de taquin slide,
  decreases the surviving labels, and fills the exit corner with the size.
  Evacuation repeatedly uses the same deletion and fills exit corners in
  decreasing order. Both operate on ordinary increasing standard tableaux.
  Promotion to the power of the size is tested for rectangles only.
- LR tableaux fill ν ÷ λ with content μ, with weak rows, strict columns, and a
  lattice reading word read right to left in successive rows from top to bottom.
  These tableaux supply both coefficients and Schur multiplication.
- Characters mean ordinary complex irreducible characters of the symmetric
  group, computed by Murnaghan–Nakayama on the permutation's cycle partition.
  Dimensions use the existing hook formula. No modular or Hecke representation
  mode is advertised.
- Symmetric functions are finite rational sums with partition indices. The five
  bases are Schur, power-sum, monomial, complete homogeneous, and elementary.
  Conversion uses character expansions, Kostka numbers and exact matrix
  inversion. Hall–Littlewood and Macdonald placeholders are removed.
- Specialization evaluates on a finite integer alphabet, yielding an exact
  rational number. This replaces the former unspecified polynomial contract.
  Empty alphabets and constant functions have their ordinary meanings.
- Plethysm is outer ∘ inner, via power-sum substitution pₖ ∘ pμ = pₖμ.
  Coefficients are rational constants, fixed under these substitutions.
  General finite sums and constants work, not just single Schur terms.
- Branching adds/removes one box. Path enumeration lists all saturated paths
  from λ to ν, including both endpoints. Empty-to-ν paths correspond to SYT.
- Permutations use one-line notation with alphabet 1 through the size.
  Subsequences return values in their original relative order. A reduced word
  consists of one-based adjacent-position transpositions, applied left to right
  to the identity. Bruhat relations mean the strong order, first ≤ second.
- Random permutations use unbiased Fisher–Yates draws. Uniform SYT generation
  chooses a removable corner with weight equal to the hook count of its smaller
  shape. Plancherel sampling takes the RSK shape of a uniform permutation.
  All share an explicit SplitMix64 state and rejection sampling for bounded
  draws. This is a reproducible noncryptographic generator. Editing the seed
  restarts the stream; successive requests advance it. Failures preserve state.

## Entering exact functions

Everything needed can be entered with the current numeric keyboard.

| Code | Basis |
|---|---|
| 0 | Schur |
| 1 | Power-sum |
| 2 | Monomial |
| 3 | Complete homogeneous |
| 4 | Elementary |

The grammar is `basis; coefficient; partition; coefficient; partition …`.
A coefficient is an integer or `numerator,denominator`. For example
`0;1;2,1` is s[2,1], and `1;1,2;2;1,2;1,1` is
½ × p[2] + ½ × p[1,1]. `0` alone is the zero Schur sum, and
`0;1;[]` is the constant 1. Destination basis is a code. Plethysm has a second
function field; specialization has an integer alphabet such as `1,2`.
Malformed input, zero denominators and trailing separators are errors.

## Bounds and evidence

Algebra has maximum degree 8 and at most 32 terms, using checked exact signed
64-bit numerators and positive denominators no greater than INT64_MAX.
LR enumeration retains at most 32 tableaux. Graph enumeration retains at most
128 paths, each with at most 16 edges. Permutations have at most 64 letters,
and reduced words at most 256 transpositions. Uniform SYT weights must fit the
existing uint64 hook-count boundary. Oversized results report an explicit error;
the application does not display a partial answer as the complete result.

The five ordinary filling kinds remain checked native properties. Specialized
skew fillings use their own API. Shifted, ribbon, oscillating, K-tableau,
column-insertion, modular-character and parameter-dependent symmetric-function
choices are not advertised. Defensive rejection of legacy unsupported values
remains distinct from a missing operation.

The correspondence executable checks every inventory signature against the
type sketch and invokes every C dispatch case. Consolidation tests additionally
check rendered results and errors, exhaustive permutations through size 6,
character orthogonality through size 6, basis round trips through degree 5,
LR products of all shapes of sizes up to 3 against independent power-sum
convolution, empty-to-shape paths through size 6, all strong Bruhat comparisons
in S₄ against a transitive cover graph, and generator support/distributions.
The native workflow also runs address/undefined sanitizers, the pinned Lua
facts tests, and signed NDK ARMv7 and C67 AArch64 packaging. The Idris file
remains a correspondence sketch; textual/native checks do not claim Idris
type-checker execution or physical-device acceptance.

Reference conventions and executable examples:
[Sage tableaux](https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/tableau.html),
[Sage symmetric functions](https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/sf/sfa.html),
[Sage symmetric-function realizations](https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/sf/sf.html),
[Sage permutations](https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/permutation.html).

The Pauli/Schur–Weyl visualization experiment in issue #7 is independent of this
ordinary mainline surface.
