# Sun Y1 — Build the initial Young Tableaux exploration APK

Use high reasoning.

## Repository

`functorial-games/young-tableaux`

Audited base:

`5a19647fe744a0eec26c1e5b187b8db93cedc02c`

Refresh the repository first. Do not assume this SHA is still HEAD if newer work exists.

Relevant in-repo design documents:

- `types/YoungTableaux/Types.idr`
- `topics/input-space.md`
- `notes/first-apk-control-surface.md`

## Reusable Android precedent

Use the existing native Android software-rendering work rather than introducing Sokol.

Primary reference:

`Ashtray-Archer/utilities-android-phone-user@ec022aea6fe6836ea78f479b498e85b3452d88b7`

especially:

- `accelerometer/app/src/main/c/native_main.c`
- `accelerometer/build-apk.sh`
- `accelerometer/app/src/main/AndroidManifest.xml`

That code already demonstrates:

- `android.app.NativeActivity`
- `ANativeWindow_lock` / `ANativeWindow_unlockAndPost`
- RGBA8888 software rendering
- pixel writes
- filled rectangles
- antialiased 5x7 glyph rendering and text measurement
- app/window/focus lifecycle
- APK packaging
- ARMv7 / AArch64 / x86_64 precedent

Also inspect current `isomorphisms/android-NDK` (audited main at `d4a4719fb97e8031476bf822697dce8423bc9030`) for reusable Android/NDK build or platform material before duplicating infrastructure.

Do not add Sokol merely to draw this first screen.

## Goal

Produce the first installable Young Tableaux Android APK.

This is **not** a single-purpose "tap boxes to build a partition" toy.

The first APK is an **exploration console** for the mathematical input/output space. The user is learning what the constructions are and what data they consume. Expose a large scrollable surface of controls, grouped into mathematical sections, so the phone itself becomes a browsable executable index of the subject.

The initial renderer can remain extremely primitive. Mathematical outputs may be text, blocks, dots, rows of integers, coefficients, or paired small diagrams.

## UI shape

Create one vertically scrollable screen, divided by clear horizontal separators, with sections at least for:

1. partition / Young diagram
2. tableau
3. hooks / corners / addable and removable cells
4. RSK
5. jeu de taquin
6. Littlewood–Richardson
7. symmetric group / representations
8. symmetric functions
9. Young graph / branching
10. random / asymptotic experiments
11. type-A / Coxeter bridge
12. global conventions

The screen may be long. That is intentional.

Use a small low-level control vocabulary such as:

- label
- separator
- text/integer field
- +/- stepper
- choice selector
- action button
- output text
- output diagram

The semantic identity of a control must remain separate from its drawn rectangle.

## Required interaction

Add the generic interaction layer missing from the accelerometer example:

- touch-down / move / up
- hit testing from screen coordinates to a stable control ID
- vertical scrolling
- focus for editable controls
- enough text/integer editing to change simple mathematical inputs
- buttons and choice/stepper controls
- redraw after state changes

Do not build a general GUI toolkit. Implement only what this APK needs, but factor the low-level pieces so they are not mixed with tableau mathematics.

If Android IME integration becomes disproportionately expensive, use a small on-screen input method for the first slice rather than abandoning editable fields. Record the limitation explicitly.

## Mathematical behavior required for v0.1

The control surface should expose the broad inventory from the Idris sketch even where implementation is incomplete.

At minimum, make the following operations genuinely executable:

### Partition

For a partition `lambda`:

- parse/edit it
- display it as blocks or dots
- validate weakly decreasing positive rows
- size `|lambda|`
- conjugate partition
- list cells
- identify corners
- identify addable cells
- identify removable cells
- hook length at each cell
- hook product
- number of standard Young tableaux `f^lambda` via the hook-length formula for values that fit the chosen exact integer representation

Overflow must fail visibly rather than silently wrap.

### Simple tableau validation

Allow entry of a small tableau/filling and report at least:

- shape compatibility
- whether entries form a standard Young tableau
- whether rows/columns satisfy the selected standard convention

Do not pretend to implement semistandard/skew/shifted cases unless they actually work.

### One RSK path

Implement ordinary RSK for a permutation using one explicit documented convention.

Input: permutation.

Output: `P` and `Q` tableaux.

The screen must state the convention being used.

This is enough to prove that the same primitive output layer can show a nontrivial transformation.

### Unimplemented operations

Controls for other typed operations are allowed and encouraged.

When invoked, they must not silently do nothing. Show something like:

```text
NOT IMPLEMENTED
input: (Partition, Partition, Partition)
output: Nat
```

or the closest readable representation derived from the documented interface.

## Idris role

The Idris file is a **type/design sketch**, not a requirement to make Idris own the executable implementation in this job.

Preserve and refine it where useful, but do not block the APK on compiling Idris to Android.

If the executable code is C for this first slice, keep the C structures and operation IDs aligned with the Idris vocabulary and document any mismatch.

Do not delete broad constructors from the Idris sketch simply because v0.1 does not implement them.

## Rendering boundary

Keep these layers separate:

1. Android raw input/window lifecycle
2. low-level controls/layout/hit testing
3. semantic control events
4. mathematical state and operations
5. output projection
6. pixels/glyphs/rectangles

The mathematical code must not call `ANativeWindow` directly.

The renderer must not know what RSK or Littlewood–Richardson means.

## Build

Follow existing project policy:

- use Ick where it is already mature enough for the required build;
- otherwise use the Android NDK path and state the precise Ick gap rather than inventing an Ick solution;
- no Gradle dependency unless there is a concrete unavoidable reason;
- preserve a reproducible command-line build.

Primary physical target:

- MIRO A1
- Android 14 / API 34 device
- `armeabi-v7a`
- 32-bit ARM

The APK must include an ARMv7 build suitable for the MIRO A1.

It is fine to keep the multi-ABI precedent if cheap, but do not let extra ABIs delay ARMv7 acceptance.

## Tests

Add host tests for the mathematical core independent of Android rendering.

At minimum test:

- valid and invalid partitions
- conjugation
- corners / addable / removable cells
- known hook-length tables
- known `f^lambda` values
- malformed/overflow behavior
- standard-tableau validation
- several small RSK permutations with known `P,Q`
- control hit-testing
- scroll bounds

Do not count screenshots or a successful build as mathematical correctness evidence.

## Acceptance

The job is complete when the repository contains:

1. source for the native Android app;
2. a reproducible build script;
3. host tests passing;
4. an ARMv7 APK artifact produced from the current source;
5. a long scrollable control surface visible at launch;
6. working touch/hit testing and scrolling;
7. editable simple inputs;
8. live partition operations listed above;
9. one working permutation-RSK path;
10. explicit visible handling of unimplemented operations;
11. documentation of the renderer provenance/reuse and any Ick gap;
12. no Sokol dependency.

If physical MIRO A1 installation cannot be performed from the execution environment, stop at a clearly identified **device acceptance pending** boundary. Do not claim physical-device success from emulator or build evidence.

## Delivery

Work on a branch and open a PR against `main`.

In the PR body include:

- exact base and head SHAs
- build command
- test command and results
- produced APK path/artifact
- ABI evidence
- implemented operation list
- unimplemented-control behavior
- renderer reuse/extraction summary
- Ick status/gap
- whether physical MIRO A1 acceptance was actually performed

Do not redesign the project beyond what is needed to get this first APK working.
