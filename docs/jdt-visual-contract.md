# Jeu de taquin: computation, projection and pixels

## Physical observation

On 8 October 2026 the MIRO C67 launched Young Tableaux 0.4.0. The user reported
that the jeu de taquin action did not change the adjacent Wegert plot. That is
**not, by itself, a computation failure**. It does expose ambiguous UI language
and a missing actual-pixel acceptance test.

Current code facts:

- `refresh_partition()` derives `Console.wegert` from **top-level λ**, namely
  `n(λ)` and the multiset of hook lengths of λ.
- The existing four reference WEGERT views use `Console.wegert`. The final
  JDT-section WEGERT now uses `Console.jeu_wegert`, independently derived from
  the **current jeu outer shape**. STEP moves the hole and values in
  `Console.jeu`; the outer shape changes only when a slide finishes.
- A forward JDT move need not change λ; the visual principal specialization
  of fixed λ therefore must not arbitrarily animate with each step.
- The *jeu board* should update, including the moved number and hole mark. A
  label or error message changing is not sufficient proof of computation.

For λ=(3,2,1), μ=(1), visible rows `1,3;2,5;4`, inner corner `(1,1)`:

1. STEP begins a hole at (1,1) and moves entry 1 left from (1,2).
2. STEP moves entry 3 left from (1,3).
3. STEP removes the outer corner (1,3), leaving ordinary shape (2,2,1),
   entries `1,3;2,5;4`, and no active hole.
4. The **reference** `s_(3,2,1)` hook-based Wegert plot is exactly unchanged
   throughout. The JDT-specific plot updates after step 3, when its outer
   shape becomes `(2,2,1)`; no spurious color change on the earlier steps.
5. A deliberate λ edit to `(2,1)` causes *recalculation* of the hook-based
   function and visibly different image pixels.

## What the tested program observes

`tests/jeu_visual_contract.c` invokes production `console_event`, verifies the
actual `Console.jeu` and `TileProjection`, verifies that `console_layout` carries
these exact projection pointers, invokes production `paint_controls`, and
compares **whole isolated paint regions with identical pixel dimensions**.
This avoids false passes caused by scrolling, added labels, page reflow or
merely hashing two strings. The positive controls demand nonzero changed board
pixels, fixed-λ **reference** Wegert pixel equality, a changed **jeu-outer** Wegert
image after an actual corner removal, and a changed reference image after an
explicit λ edit.

`tests/jeu_visual_contract.py` runs the real test and separately compiles four
real C mutations: bypass the STEP, disable DIAGRAM rasterization, suppress
Wegert invalidation on λ edit, and freeze the jeu-outer projection after its
initialization. Every mutation must produce the diagnostic
`JDT_VISUAL_FAIL` rather than a false pass. The runner requires an explicitly
chosen compiler. `test-host.ysh` integrates the positive contract into the
existing exact-host ICK lane; the standalone visual CI uses an exact pinned NDK
host compiler and collects all four mutation records.

## First Idriç attempt, intentionally narrow

`types/YoungTableaux/JeuTrace.idric` implements a pure neighbor-decision model
and distinct observation types for an ordinary skew board, an in-flight hole,
and the source of each projection. `JeuTraceChecks.idric` supplies thirteen checks
including the semistandard tie convention (equal neighbors choose below).
These are not yet a verified full Idriç JDT algorithm. Shape/corner admission,
arrays, the actual native state mutation, compiler checking and Android lowering
are separate obligations. Do not replace the working native engine from this
source-level attempt or claim a typechecker run without a receipt.

The independent audit's ten documented failures, including oversized-cell
validation and plot arithmetic overflow, remain tracked; this focused test
neither suppresses nor resolves them. This is not a release acceptance record.
