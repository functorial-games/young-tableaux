# Operation definition audit

This audit covers all 44 entries in `app/console/operations.def`.

The registry is checked in three independent ways:

1. `types/YoungTableaux/Types.idr` must contain exactly one matching
   `InputFor` and `OutputFor` clause for every registry operation.
2. `types/YoungTableaux/Operations.idric` must contain a matching Edriç
   specification for every registry operation.  Each operation also has a
   named snake_case `*_signature : Type` definition that exists independently
   of backend implementation status.
3. `tests/operation_correspondence.c` initializes a fresh console and invokes
   every registered C dispatch path.  Empty output, `INTERNAL DISPATCH`, and
   `NOT IMPLEMENTED` are failures.

The ordinary mathematical owners remain narrow:

| Surface | Native owner | Principal checks |
|---|---|---|
| partitions, cells, hooks, add/remove | `partition.c` | partition validity, exact hook/count arithmetic, legal corners |
| fillings, standardization, insertion, reverse insertion, transpose | `tableau.c` | shape compatibility, row/column conditions, checked insertion traces |
| permutation/word/biword/matrix RSK | `rsk.c` | typed input families, shared row insertion, equal P/Q shape, checked completed-result families |
| inverse RSK, promotion, evacuation | `combinatorics.c` | standard-tableau boundaries and existing insertion/jeu primitives |
| jeu de taquin and rectification | `jeu_de_taquin.c` | removable inner corners, visible hole state, weak rows/strict columns, lower-entry tie rule |
| LR, characters, symmetric functions | `algebra.c` | lattice words, Murnaghan–Nakayama, exact rational basis conversion/specialization/plethysm |
| Young graph, random generation, LIS/LDS, Coxeter, Bruhat | `combinatorics.c` | bounded exact results, explicit RNG state, independent graph/order checks |

The consolidation suite adds independent mathematical evidence rather than
merely checking that dispatch returns text: exhaustive small permutations,
character orthogonality, basis round trips, LR cross-checks, Young-graph path
counts, strong Bruhat closure, random support/distribution checks, and error
preservation at documented bounds.

This is strong executable verification, not a formal proof that every
implementation is mathematically correct.  The Edriç specifications make the
intended domain and codomain explicit even for future operations that may be
specified before a backend is written.
