# Functorial C repair / qualification — 2026-10-06

The ordinary registered operation surface is now executable through typed
native boundaries. See [the consolidated contracts](consolidated-operations.md)
for exact semantics, finite bounds, shared owners and checks. The correspondence
gate checks sketch signatures and invokes every registered C operation. The
type sketch itself has not been executed through an Idris/Idriç checker.

The remainder below records the earlier 0.3.3 qualification; its missing-operation
and inventory observations are historical, superseded by the consolidated
ordinary contracts and narrowed public modes.
Application ICK compilation is also blocked on Bionic nullability and Android
availability annotations, reproduced with current source-built ICK at API 26.

| Construction | Native owner / composition |
|---|---|
| Partition, Cell | `partition.h/c`, checked corner addition/removal, conjugation and hook operations |
| Young graph state | `DiagramState`, checked add/remove → partition operations, undo/reset of full Cells |
| Straight fillings and kinds | `tableau.h/c`, shared shape, row/column order and alphabet predicates; supported kind validation |
| Permutation and Word | Distinct structs and parsers; repeated letters cannot pass permutation validation |
| Biword | Paired `Biletter` values with equal-length and lexicographic validation |
| Natural matrix | Rectangular nonnegative `NatMatrix`; expansion → biword → shared insertion |
| RSK | `rsk.h/c`: checked input → `tableau_row_insert_trace` → `record_insertion` → trace/result |
| Completed RSK families | `PermutationRSKResult` (standard/standard), `WordRSKResult` (semistandard/standard), `BiwordRSKResult` (semistandard/semistandard, also the matrix image); checked constructors require complete traces and equal shapes |
| Skew filling / slide | `SkewShape`, `SkewTableau`, separate `JeuState`; select smaller neighbor → move into hole → finish at outer corner |
| Interaction and rendering | Console parses operation-specific values; `TileProjection` remains a derived display object |

The monolithic math file is removed. Repeated bumping arithmetic is removed;
trace and ordinary insertion use the same owner. Direct C interactions and the
Lua result-validation path use native `DiagramState`; no bridge copy of corner
arithmetic/history replay owns the state. A candidate Lua result is compared
with a native transaction before native state is installed.

Insertion/content conventions and tableau kinds now have named native enums.
Column insertion is explicitly rejected by insertion and reverse-insertion
interactions rather than executing row insertion. Supported filling kinds are
checked properties, not trusted tags. The skew slide state cannot be passed
where a completed filling is required, or vice versa, through matching pointer
types. Private search, bumping, shape growth and slide helpers remain private.

The sketch now records positive partition rows, permutation uniqueness/range,
positive words, paired biwords, rectangular matrices, DiagramState's base, and
the filling/slide distinction. Operation metadata has exact executable tests
against every sketch input/output; pre-existing NumberMatrix/NatMatrix and
Number/Nat drift is repaired. These are textual correspondence tests: the
changed Idris sketch was not executed through an Idris/Idriç type checker here.

Executed: 6,735 regression checks including convention and completed-result checks,
insertion-owner interception, matrix/biword equivalence and malformed-input
tests, output-preserving rejection, three negative C type fixtures, and a mutated
sketch-signature rejection. A compiling mutation weakening the standard-result
constructor to semistandard is rejected by a runtime assertion. Real Lua facts
tests pass using the live branch's
symbolic fork pin `dca7e57c16c524c8616144ed294fe598947a029f`. The changed Android
tableau, RSK, console, entry and bridge units compile with NDK r27c A32 API 26;
APK packaging and physical acceptance were not executed.

Completed interaction results now pass the corresponding checked result
constructor before rendering; partial insertion traces deliberately retain
general fillings because a permutation prefix need not have alphabet 1..k.
The sketch and operation table use the distinct completed result types, with
the inverse operation restricted to the permutation result family (its inverse
algorithm remains unavailable). Runtime tests reject wrong alphabets, partial
traces, mismatched shapes, malformed bounds and semistandard recording results
where standard results are required. A third negative C fixture rejects a word
result at a permutation result boundary.

Remaining structural boundary: `Request`/`OutputFor` still describe a broader
family than the native checked APIs.
Shifted/ribbon/oscillating/K objects are still inventory entries, not faithful
executable representations. Choosing their shape/state constructors and the
correspondences for these additional objects is a mathematical model decision; unsupported
operations remain explicitly unavailable. No generic command bag is certified
as an implementation of those constructions.
