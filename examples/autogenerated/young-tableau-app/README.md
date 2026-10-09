# Young Tableaux in Idriç

This is the executable Idriç rewrite attempt on the `Idriç` branch. The complete
44-operation mathematical registry, application model, command input, picture
production, and presentation protocol are implemented in `.idric` sources.
The host program produces and reads back its own pictures. The Android
application is **blocked at target lowering**; this directory does not contain a
new APK or evidence from a phone. See [the actual backend probes](native-boundary.md).

The work starts from `7728a9c8518337a405e09b53b8f2372d35bc08c4`, which contains
the latest native jeu de taquin and typed-pixel work. It adds an independent
application directory without replacing the existing native implementation.

## What changes visibly

The previous console source initialized `rsk_step` to `YT_DIM`, refreshed the
completed trace, and allowed Next to increment only below `rsk_total`. Thus its
initial Next action had no remaining insertion to perform. This is a source
finding, distinct from a diagnosis of the particular APK installed on a phone.
See the [starting console implementation](https://github.com/isomorphismes/young-tableaux/blob/7728a9c8518337a405e09b53b8f2372d35bc08c4/app/console/console.c).

RSK starts before its first insertion. Next inserts one letter and repaints P,
Q, the insertion path, the input position, and the sequence picture from the
same model. Previous reconstructs the preceding prefix. Loading a new word,
permutation, ordered biword, or nonnegative matrix starts at the beginning.
The explicit `RSK*` laboratory commands instead return their complete result.

Jeu de taquin maintains its visible entries and its hole separately. Step moves
one entry into the hole; Slide finishes the selected slide; Rectify processes
the remaining skew shape; Reset restores the supplied skew tableau. Equal
neighbors choose the entry below. The selected inner corner is shown explicitly.
Controls that cannot act are disabled, and the command interface reports an
explicit error for an invalid action.

The two Wegert pictures have different inputs. The fixed-partition picture
depends on the selected reference partition. The current-jeu picture depends
on the current outer shape and changes when the slide removes its exit corner.
An intermediate hole movement changes the jeu board while preserving both
unchanged-shape pictures. This is deliberate mathematical behavior.

Every laboratory command stores its actual returned data in an inspection
view. Returned partitions, tableaux, RSK pairs, and jeu fillings also update
their corresponding main panes. The last operation's inspection is labeled as
such. It retains the entire result, with ten wrapped text rows and two diagrams
per page; Previous/Next result page exposes all pages without constructing an
unbounded image. Random commands consume and advance the model's SplitMix64
state, and `seed` resets that stream explicitly.

## Program ownership

| Source | Responsibility |
| --- | --- |
| [Young/Core.idric](Young/Core.idric) | Partitions, tableaux, insertion, all four RSK inputs, inverse permutation RSK, jeu de taquin, promotion, evacuation |
| [Young/Advanced.idric](Young/Advanced.idric) | LR tableaux and coefficients, Schur products, characters, exact rational symmetric functions in five bases, specialization, plethysm, branching, paths, permutations, and random generation |
| [Young/Laboratory.idric](Young/Laboratory.idric) | All 44 operation requests, their dependent result types, checked evaluation, and projection of actual results |
| [Young/Model.idric](Young/Model.idric) | One authoritative model and checked actions; successful actions advance its revision |
| [Young/Commands.idric](Young/Commands.idric), [Young/TextInput.idric](Young/TextInput.idric) | Strict command and mathematical input grammars |
| [Young/Inspection.idric](Young/Inspection.idric) | Complete result pagination with bounded frame size |
| [Young/Raster.idric](Young/Raster.idric), [Young/Plots.idric](Young/Plots.idric), [Young/Screen.idric](Young/Screen.idric) | Deterministic pixels, mathematical pictures, layout, and source-indexed frames |
| [Young/Presentation.idric](Young/Presentation.idric) | Pointer dispatch, current-window identity, pending frames, and acknowledgment of actual presentation |
| [Main.idric](Main.idric) | Host command runner, combined checks, and a verified image-file presentation sink |

The mathematical layer uses exact integers and rational coefficients. The
Wegert sampler uses host binary64 arithmetic, including the existing CIE Luv
color rule; it makes no Float16, GPU, or Android rendering claim. Mathematical
types do not inherit display array dimensions. Interactive admission is
explicit: 64 rows, 64 columns, 256 cells, and 64 input letters. The bounded
algebra surface has degree 8, 32 terms, 32 retained LR tableaux, 128 paths of at
most 16 edges, and 256 Coxeter generators. Exceeding a supported bound returns
an error instead of silently presenting a partial result as complete.

## Model, pixels, and acknowledgment

`paint : (source : Model) → Frame source` owns frame construction. Consumers
can read its raster but cannot relabel an old source's frame or invent a frame
from arbitrary pixels through the exported API. These are compile-checked
interface constraints, not a proof of the painter's mathematical correctness.
The production painter is also exercised by pixel and mutation checks.

An offered frame carries its real window identity and surface generation.
Acknowledgment succeeds only after the output callback reports success and
the complete source model, window identity, and generation still match. A
failed lock or post remains pending. Losing a window preserves the model;
recreating the window requests the current picture. Equal revision numbers
alone are insufficient: different mathematics, inspection contents, pages, or
random states cannot acknowledge another model's frame.

The host callback writes the provided pixels as a PPM file, reads that file
back, checks every byte, and only then acknowledges it. It is a real host image
sink. An Android callback that locks, fills, and successfully posts a native
window still has to be implemented through a working Idriç target backend.

## Compile and run

The maintained host qualification uses
`dilapidated-shed/Idric@ff4d852862a3942592f8ade9afde8d409d9803be`
with its already-built Chez compiler and installed base/prelude. It does not
bootstrap a compiler or install another implementation. Use the actual Grease
program and replace every `/ABS/...` path below with an absolute path. The output
directory must not exist yet.

```grease
grease /ABS/young-tableaux/examples/autogenerated/young-tableau-app/_/qualify-host.grease /ABS/young-tableaux /ABS/Idric /ABS/chez/bin/scheme /ABS/new-output
```

The qualifier builds a fresh executable, checks diagnostic output as well as
exit status, runs all host assertions, writes eleven sample frames, checks two
intended type refusals, and compares source hashes before and after the run.
Its output executable is `new-output/exec/young-idric`.

```grease
/ABS/new-output/exec/young-idric --render /ABS/rsk-and-jeu.ppm next next step
```

Each multiword operation is one quoted argument. Commands run sequentially
against the state produced by the preceding command.

```grease
/ABS/new-output/exec/young-idric --render /ABS/graph-page.ppm 'EnumerateYoungGraphPaths [] 3,2' result-next
```

See [commands.md](commands.md) for all 44 command examples, input grammar, basis
codes, random-stream control, and result paging. PPM files can be read by a
standard image viewer or losslessly converted to PNG.

The companion mutation qualifier makes isolated copies, compiles each altered
application successfully, and requires its particular behavioral assertion to
fail. A compile error or unrelated file-writing error does not count.

```grease
grease /ABS/young-tableaux/examples/autogenerated/young-tableau-app/_/qualify-mutants.grease /ABS/young-tableaux /ABS/Idric /ABS/chez/bin/scheme /ABS/new-output /ABS/new-mutant-output
```

## What was learned from Fourier-sound

The inspected source is
[`isomorphismes/Fourier-sound@ac0e9b40a556012b42be240f1c32c88370028b4a`](https://github.com/isomorphismes/Fourier-sound/tree/ac0e9b40a556012b42be240f1c32c88370028b4a).
Its native-window output checks inspect actual pixel bytes and failure paths;
its media fixture carries signal data through rendering and file readback;
its Android frame owner only advances after a successful post. Arithmetic
tests alone do not establish visible output.

The corresponding checks here follow pointer input into the real reducer and
production painter, then compare the cropped P, Q, and jeu boards. They exclude
headings and counters from the decisive crops. They also cover the fixed-shape
plot invariant, changed-shape plot output, stale-source acknowledgment,
window loss/recreation, failed posting, and no-loss result paging. The final
receipts and their exact evidence boundary are recorded in
[the attempt record](attempt.md) and [the verification record](verification.md).

## Remaining Android work

The current direct DEX implementation emits a fresh integer-addition positive
control but refuses the checked recursive helper needed by the model and the
tested dynamic array/IO boundary. The separate native ARM implementation emits
its Float32 positive control but refuses the integer result probe. These are
actual backend refusals, not inferred from the host program.

The next step is to extend and execute those small backend probes, then lower
the maintained model and painter and supply Android event and presentation
effects. APK construction, ART execution, successful Android window posting,
and observation on the user's physical device remain uncompleted. The existing
C app is not silently used as the implementation of this Idriç attempt.
