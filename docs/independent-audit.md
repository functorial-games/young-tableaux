# STAR YT-AUDIT: independent mathematics and public dispatch

Source anchor: `b2ffa7b25a415a550512c1ffb4eabfb99daec8c3`, re-fetched from
`functorial-games/young-tableaux/main` on 2026-10-07. Audit branch:
`star/yt-independent-audit`. No production mathematical or UI source is changed.
No merge, release, or physical-device acceptance is authorized or claimed.

The initial live inventory included open PR #16, **Add an insertion-sequence
plot beside RSK**, head `95611b1e8a356bfbaa6cd8a10e9b416474d4d2ef`.
It is separate from this audit. Merged PR #15, **Make RSK and jeu de taquin
visual and add Edriç contracts**, supplied the audited visual traces. Merged
PR #13, **Complete the non-Pauli Young Tableaux operation surface**, supplied
the consolidation suite. All available open/merged PR metadata, branches,
workflows and source were refreshed. No AGENTS.md or STYLE.md was present in
the checkout or its workspace ancestors.

## How to interpret PASS

PASS means independently checked in the explicitly stated finite range, plus
the public-dispatch fixture in `public-dispatch.tsv`. It is not a proof over
every machine integer, every UI interaction, or every physical device.
The four permitted statuses are used literally in the TSV ledger. A failing
validation or output route overrides passing mathematical-owner evidence.
Internal helpers are not marked broken merely because their buttons were
intentionally hidden. The layout exposes **27 operation buttons**, not 44;
sections 0–2 are internal/derived or direct-diagram controls. The registry still
advertises their public console dispatch contracts.

No incorrect mathematical value was found for a well-formed input within the
exhaustive ranges. Eight concrete validation/output counterexamples were
found. They are preserved as ordinary failing assertions, not xfails or
waivers. The audit is expected to exit 1 on this source until they are fixed.

Final local run: **171,228 checks, eight failing counterexamples**. The
44-operation result is **36 PASS, eight FAIL — UI/wiring/validation**. All
eight mathematical mutants compiled and were killed. Detailed counts and
verbatim counterexamples are in `audit-evidence/independent-audit.tsv`.

## Concrete counterexamples

| ID | Operation / exact input | Actual | Required | Owner |
|---|---|---|---|---|
| C1 | FindCorners; λ=`1` | `Hook product = 1` and `Hook-length formula gives 1 standard tableaux.` | The removable corner `(1,1)` | `console.c: console_run → refresh_partition`; computed list is discarded |
| C2 | FindAddableCells; λ=`[]` | Same two hook/count lines | The addable cell `(1,1)` | Same |
| C3 | FindRemovableCells; λ=`1` | Same two hook/count lines | The removable cell `(1,1)` | Same |
| C4 | AddCell; λ=`[]`, cell=`4294967297,1` | `add (1,1) -> [1]` | Reject unrepresentable row; no result | `console.c: selected_cell`, unchecked `%d` conversion |
| C5 | RemoveCell; λ=`1`, cell=`4294967297,1` | `remove (1,1) -> []` | Reject unrepresentable row | Same |
| C6 | ReverseInsert; tableau=`1`, cell=`4294967297,1` | `reverse bump from (1,1); ejected 1`, result `[]` | Reject unrepresentable row | Same |
| C7 | Native `jeu_rectify`: outer=`(1,2)`, inner=`()`, rows=`1;2,3`, inactive | `YT_OK`, malformed shape retained | `YT_MALFORMED`: row lengths increase | `jeu_de_taquin.c: skew_tableau_validate` omits partition validation |
| C8 | JeuDeTaquinSlide; λ=`1`, μ=`1`, skew=`[]`, cell=`4294967297,1`; RESET then SLIDE | Slide succeeds and empties outer/inner shapes | Reject unrepresentable row | `console.c: selected_cell`; shared parser with C4–C6 |

C4–C6 and C8 were executed on hosted x86_64 with the actual C parser. The out-of-range
`scanf` conversion is not a portable way to reject overflow; this report does
not claim the exact wraparound was observed on Android. `4294967297` is the
smallest positive row observed to wrap to legal row 1 on this host.
C7 is a native-API boundary failure. The console's partition parser rejects
that shape, so C7 is **not** claimed as a reproduced phone UI failure.

## Independent oracles and exhaustive ranges

All references are local deterministic Python, explicitly authorized for this
audit. They do not invoke production to compute expected answers. Native
observations use `ctypes`; `audit_bridge.c` exposes the same console/event/layout
functions used by the application without reimplementing mathematics.

| Family | Independent construction | Range |
|---|---|---|
| Partitions/hooks | Ferrers cell sets; count cells directly in each hook; enumerate SYT by largest-label corners | Every partition of sizes 0–10; all cells and legal additions/removals |
| Tableau kinds | Direct row/column inequalities and alphabet sets, both increasing/decreasing modes | Every filling over 1,2,3 of every partition through size 5; transpose; semistandard standardization |
| Insertion / reverse | Simple Python row lists; first entry strictly greater than inserted value | Every word over 1,2,3 through length 6, every prefix; reverse insertion reconstructs prefix and ejected letter |
| Permutation RSK | Independent insertion, P/Q values; inverse/inversion and reverse-shape identities | All 5,914 permutations of sizes 0–7; 5,040 at size 7; distinct pair counts equal n! |
| RSK word | Independent insertion with chronological Q; weak LIS and strict LDS by dynamic programming | All 1,093 words over 1,2,3 through length 6 |
| RSK biword/matrix | Lexicographic pair expansion and independently reconstructed P/Q | All 81 2×2 matrices with entries 0,1,2, including zero matrix, repeated biletters, ties; matrix-transpose P/Q exchange |
| Visual RSK | Independently computed before/after tableaux, full bump marks and Q new-cell mark | All word prefixes above at backend; one multi-step public fixture per input family, Start/Prev/Next/End and edit switching |
| Jeu de taquin | Dictionary of occupied cells; minimum right/below move with below on ties | All skew shapes with outer size ≤5; all semistandard fillings over 1,2,3, plus all standard fillings; every legal first corner and every complete slide order |
| Promotion/evacuation | Delete 1 and dictionary slides; place exit labels independently | Every SYT of every partition of size 0–7; exact comparison and evacuation involution |
| LR | Enumerate complete multiset fillings, then separately filter rows, columns and every reading-word prefix | All triples with outer size ≤6 and size-compatible inner/content; full filling sets, duplicate counts, zero coefficients and symmetry |
| Schur multiplication | LR brute force, plus direct polynomial Jacobi–Trudi determinants | All factors of total degree ≤6; independent polynomial products through total degree 5 |
| Characters | Jacobi–Trudi alternating sum of permutation-module characters, counting assignments of entire cycles to labelled capacities | Complete tables S₀–S₈; row AND column orthogonality, sign/trivial rows, identity dimensions from independently enumerated SYT |
| Symmetric functions | Direct polynomial h/e/p/m definitions and Schur determinants through degree 5; independent power coordinates via set-partition Möbius inversion for monomials and independent characters for Schur | Every singleton in all five bases to every destination, degrees 0–8; exact rational mixed sums |
| Specialization | Evaluate independent polynomials; Weyl dimension product as a second degree-8 oracle | Every basis element through degree 5 at empty, zero, negative, repeated and multi-element alphabets; all degree-8 Schur elements at 1⁸ |
| Plethysm | Independent Adams substitution fixing rational constants | Every positive-degree pair of singleton basis elements from all five bases whose degree product ≤8; constants, zero, unit, rational constants and mixed sums |
| Young graph | Cell-set adjacency and independently enumerated path tuples; compare full sets, not only counts | Every partition through size 7; empty-to-shape paths; >128-path explicit rejection; SYT correspondence |
| Random | Independently enumerated support and exact target probabilities | Seed 20261007; 24,000 samples EACH of S₄ permutations, SYT(3,2,1), and size-4 Plancherel shapes |
| LIS/LDS | Independent quadratic dynamic programming plus subsequence membership/ordering | Every permutation through size 7; word identities checked separately with weak LIS/strict LDS |
| Coxeter | Direct inversions and multiplication; recursive independent complete reduced-word sets | Every returned word through S₇; membership in complete independently enumerated reduced-word sets through S₅. App advertises ONE reduced word, not enumeration |
| Strong Bruhat | Subwords of an independently found reduced expression | Every pair in S₀–S₅, including all 14,400 pairs in S₅; production uses rank matrices, not transitive closure |

The RNG tests use χ² < 80 as a conservative deterministic regression threshold
for each distribution (23, 15 and 4 degrees of freedom respectively).
Observed χ² values: 21.134, 19.2226666667, 2.3284444444. Each draw's support
is checked independently. These are seeded statistical tests, not a proof of
uniformity over all 2⁶⁴ initial states. The C source's rejection threshold and
corner-count weights were also inspected; weights agree with independent SYT
counts in the partition range. Meaningful multi-category bias is detected;
arbitrarily small bias is not excluded.

Boundary cases include empty partitions/tableaux, 64-entry increasing and
decreasing permutations, 64 rows/columns, 256 cells, 257-cell rejection, hook
product and dimension overflow, degree-9 rejection, constants, zero, and
malformed public inputs. These are named boundary fixtures, not an exhaustive
fuzzing claim for every malformed byte string or every size-limit combination.

The reference conventions agree with the published Sage documentation:
[tableaux](https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/tableau.html),
[symmetric functions](https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/sf/sfa.html).
No Sage executable was run; these documents are convention references, not
an invented external execution receipt. Integer-labelled ordinary tableaux
and positive-alphabet RSK are distinguished, as are permutation-only inverse
RSK and the word/biword result families.

## Existing evidence: useful, but insufficient

- The old correspondence executable only requires nonempty non-placeholder
  text. An input error or unrelated hook count satisfies it. C1–C3 pass it.
- Its Idriç check reads `-- SPEC` comments, not the actual declarations.
  Changing only `rsk_permutation_signature` from `permutation` to `word`
  leaves the old gate green. The new audit checks all actual `input_for`,
  `output_for` and named signatures (132 comparisons). This remains textual
  checking, not external Idriç/Idris type-checker acceptance.
- Basis round trips reuse production conversion in both directions. They can
  preserve a mutually consistent wrong convention or matrix.
- The old LR product convolution is independently multiplied, but its source
  and result coordinates come from production character/basis conversion.
  Its LR list/count comparison also shares one production enumeration.
- Old character checks contain useful S₃ fixtures and row orthogonality, but
  no separately constructed full character table or column orthogonality.
- Old inverse-RSK, evacuation and graph-count identities are useful laws but
  share insertion, slides, hooks or adjacency owners.
- The existing S₄ Bruhat transitive-cover oracle IS independent of the
  production rank matrix. It was not falsely labelled circular; this audit
  adds a different subword oracle through S₅.
- Existing support tests/loose histograms cover only S₃ and a two-tableau
  shape; they do not by themselves establish the advertised distributions.

The unchanged legacy suites pass locally: **9,893 host assertions**,
**106,612 consolidation assertions**, operation correspondence, and the pinned
symbolic Lua facts/layout test. New counterexamples survive all those suites.

## Mutation evidence

Each baseline family must first pass. Each mutant must compile and produce a
mathematical mismatch TSV with exit 1; compiler failure or a crash is not a
kill. Sources are copied to a temporary directory and never changed in place.
Only summaries/receipts are committed, never mutant production code.

| Mutant | First killing fixture | Correct / mutant |
|---|---|---|
| Hook arm +1 | shape (1), cell (1,1) | 1 / 2 |
| RSK bump `>` changed to `≥` | word (1,1) | P=[1,1], Q=[1,2] / malformed result |
| Jeu tie chooses right | outer (2,1), inner (1), entries 1;1 | hole moves below / hole moves right |
| LR lattice condition removed | inner (), content (1,1), outer (2) | coefficient 0 / 1 |
| Murnaghan–Nakayama sign reversed | shape (1), cycle (1) | 1 / −1 |
| Young-graph edge omitted | successors of () | {(1)} / empty |
| Bruhat comparison reversed | (2,1) ≤ (1,2) | false / true |
| Inverse RSK removes Q ascending | permutation (1,2) | (1,2) / rejected |

## Reproduction and evidence boundaries

Run `tests/independent_audit.py --compiler /absolute/path/to/NDK/clang` with
Python 3. The executable emits a TSV receipt and a nonzero exit for any
counterexample. `--family` isolates a family; `--library` reuses an observed
native library. `tests/audit_mutations.py` builds disposable mutants and
requires all eight kills. `tests/audit_existing.py` reproduces old suites and
the old actual-signature blind spot. All paths resolve from the scripts,
independent of the launch directory.

Local host compilation used NDK r29 / 29.0.14206865 targeting x86_64 GNU/Linux.
The discovered r27c local directory had a dangling compiler symlink and was
not represented as a working compiler. Hosted audit CI explicitly installs
the repository's r27c / 27.2.12479018 compiler. This does not modify the paired
A1/C67 production build definitions or substitute a new production toolchain.

`independent-audit.yml` adds a separate failing mathematical gate. The existing
paired-build workflow remains separate so signed ARMv7/A1 and AArch64/C67
qualification can be reported independently of known mathematical failures.
Starting workflow evidence is recorded in `independent-audit-runs.tsv`.
New exact-head hosted run URLs/results are reported in the visible audit
handoff after publication. A green package build never overrides a failing audit.

Physical MIRO A1 and MIRO C67 acceptance: **NOT RUN**. Neither installation,
launch, touch behavior nor visual performance on a device is claimed.

Remaining unverified boundaries: external Idriç/Idris type checking, physical
UI/rendering, all malformed native-memory states, exhaustive overflow behavior,
all arbitrary rational finite sums, and the RNG distribution over every seed.
These are explicit limits on PASS scope, not silently upgraded evidence.
