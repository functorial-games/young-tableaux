# Native exploration APK v0.1

The app opens twelve sections on one vertical surface: partitions; tableaux; hooks/corners/cells; RSK; jeu de taquin; Littlewood–Richardson; symmetric group/representations; symmetric functions; Young graph; random/asymptotic experiments; type A/Coxeter; global conventions. Swipe on the surface to scroll, tap a field to edit, and tap an operation to see a result or its unimplemented typed interface.

## Executable mathematics

- Partition parsing, weakly decreasing positive rows, size, diagram, conjugation, all cells, corners/removable cells, addable cells, every hook length, hook product, and standard-tableau count. Empty partition is `[]` (also an empty field).
- Small filling parsing and shape compatibility; strict rows and columns; standard entries 1 through the number of cells, each used once. The default convention increases along rows and down logical columns; an explicit decreasing alternative reverses both inequalities. Other tableau-kind choices return `NOT IMPLEMENTED`.
- Ordinary permutation RSK, inserting the permutation left to right. Row insertion replaces the first strictly greater entry, bumps that entry to the next row, and appends if none is greater. Q places the insertion-step number in the new cell. Both P and Q use increasing rows and columns. Column insertion is explicitly unimplemented.
- Characteristic-zero Specht dimension uses the standard-tableau count. Other characteristics return `NOT IMPLEMENTED`.

For example, lambda `3,2,1` has hooks `5,3,1;3,1;1`, hook product 45, and 16 standard tableaux. The permutation `3,1,4,2` produces P `1,2;3,4` and Q `1,3;2,4`.

The implementation bounds partitions/fillings to 64 rows, 64 columns, and 256 cells, and permutations to 64 entries. Entries use checked signed 32-bit parsing; exact counts/products use unsigned 64-bit integers. Invalid input, structural limits, integer parsing overflow, and arithmetic overflow are visible errors. Errors leave output arguments unchanged. Addable cells describe the mathematical boundary even when adding the cell would exceed an executable storage limit.

The hook-length count cancels prime factors of the factorial and hook lengths before multiplying the final quotient. A factorial or hook product may overflow while the count still fits, so those errors are handled independently. Host tests compare the formula with an independent branching recurrence for every partition through size ten.

## Interaction and limitations

Fields accept decimal integers, commas/spaces, semicolons, brackets, minus signs, and decimal points using a fixed on-screen keypad. DEL removes the final character, CLEAR empties the field, DONE closes the keypad. Edits to lambda, filling, and permutation update results immediately; temporary invalid input is expected during editing. Back closes the keypad before leaving the app. Input is append/backspace editing, capped at 511 characters; Android IME, clipboard, arbitrary letters, cursor insertion, and accessibility nodes are not implemented. Textual inventory fields such as basis/law keep example defaults and can only be numerically edited in this slice. These limitations do not affect partition, tableau, or permutation entry.

Numeric fields, choice selectors, operation buttons, and an n stepper exercise the same small control layer. Control IDs are stable semantic integers, separate from their rectangles and layout order. A drag never activates a button; cancellation and secondary pointer events cannot complete another pointer's tap. Scroll bounds account for the keypad. Focus is visible and brought into view when opened. App-owned input/convention/result data survives Android saved-state recreation; scroll position is transient.

English/French choices reverse diagram row placement. Logical cell coordinates remain one-based from the longest row, independent of placement; tabular outputs always retain logical row order. Reading order, action side, content convention, non-standard tableau classes, and unimplemented construction inputs are inventory controls. They do not change permutation RSK. Unfinished operation buttons replace their section's text with `NOT IMPLEMENTED`, operation name, `input: ...`, and `output: ...`; unsupported tableau/RSK paths also hide their previous diagrams.

## Boundaries and Idris alignment

| Layer | Source | Responsibility |
| --- | --- | --- |
| Android input/window/lifecycle | `app/android/native_main.c` | NativeActivity, MotionEvent adaptation, focus/lifecycle, window lock/post |
| Controls/layout/hit testing | `app/ui/controls.c` | Rectangles, stable IDs, touch cancellation, scroll, focus |
| Semantic events and state | `app/console/console.c` | Field meanings, selected conventions, operation dispatch |
| Mathematics | `app/core/math.c` | Host-testable parsing, partition/tableau/RSK functions |
| Output projection | `app/console/console.c`, `app/ui/controls.h` | Text and generic ragged tile rows with optional integer labels |
| Painting/pixels | `app/render/paint.c`, `app/render/raster.c` | Controls, generic tiles, rectangles, antialiased glyphs |

The core never calls ANativeWindow. The renderer never dispatches mathematical operations or examines partition/RSK meanings. French row reversal is passed as a generic drawing option.

`operations.def` uses the Idris Operation constructor names and InputFor/OutputFor vocabulary. `Types.idr` retains all original constructors and adds ListCells and the symmetric-function inventory operations ChangeBasis, Specialize, and Plethysm with supporting design types. C Partition/Tableau are bounded ordinary-shape structures, not implementations of the broader Idris skew/tableau algebra. C Cell arrays are zero-based internally; cell outputs are one-based. C uint64 results cover only a checked subset of Idris Nat/Integer. Validation exposes four diagnostics where the sketch's ValidateTableau result is Bool. Idris remains a design sketch; it was not compiled to produce this APK.

## Renderer and Android provenance

Primary source: [Ashtray-Archer/utilities-android-phone-user at ec022aea6fe6836ea78f479b498e85b3452d88b7](https://github.com/Ashtray-Archer/utilities-android-phone-user/tree/ec022aea6fe6836ea78f479b498e85b3452d88b7/accelerometer). Inspected native_main.c, build-apk.sh, and AndroidManifest.xml at that exact revision.

`raster.c` extracts its pixel writes, blending, 5x7 glyph lookup, continuous interpolation/4x4 supersampling, cached masks, rectangles, and text measurement. Android's buffer type is replaced by a generic Canvas with stride and clip bounds; sensor-specific state/rendering is removed. Added missing letters/punctuation, uppercase fallback, and clipped rectangle iteration. NativeActivity, RGBA8888 window locking/posting, lifecycle, no-DEX packaging, and stable signer handling follow that precedent. New controls are app-specific, not a general GUI toolkit. No Sokol, OpenGL, Gradle, Java sources, or application DEX is used.

Also refreshed [isomorphisms/android-NDK at d4a4719fb97e8031476bf822697dce8423bc9030](https://github.com/isomorphisms/android-NDK/tree/d4a4719fb97e8031476bf822697dce8423bc9030) and inspected ARCHITECTURE.md, its inventory, and platform route. It owns generic platform mechanics; this app owns its manifest, package/library identity, UI, math, and signer choice. Use the NDK's existing android_native_app_glue rather than copying it into this repository.

Ick gap: [the refreshed Android qualification policy](https://github.com/dilapidated-shed/ick/blob/73af2ef14fd81a1a4f2cf977aea87aa9d537cad8/docs/android-release-gate.md) distinguishes focused four-ABI ordinary-C/PIC object-and-link evidence from full native application/header/sysroot-driver/runtime/APK qualification. This app's android/input.h, NativeActivity glue, pthread/lifecycle, native-window headers, and complete Android shared-library path have no qualified available Ick driver in this environment. v0.1 uses the declared NDK C compiler; no invented Ick solution or Ick acceptance claim is made.

## Reproducible command-line build

Build on a Linux x86_64 host, never on the MIRO A1. Prerequisites: Grease's actual pinned Oils-derived YSH entrypoint, host C compiler, JDK/keytool, zip, rg, Android platform 34 (revision 3), build-tools 35.0.0, and NDK r27c / 27.2.12479018. Supply absolute paths; the build needs no launch-directory assumption.

Use the established public development signer from [isomorphismes/wegert at 89dcfb840cec1a66ee04c7f7404954cbd6c09839](https://github.com/isomorphismes/wegert/blob/89dcfb840cec1a66ee04c7f7404954cbd6c09839/_/build/app/wegert-debug.keystore). Alias/password are `wegert-debug`; expected certificate SHA-256 is `de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9`. This is a public development identity, not production signing.

Replace the illustrative absolute paths with verified host locations:

```text
/absolute/grease-entrypoint /absolute/young-tableaux/test-host.ysh /absolute/young-tableaux
/absolute/grease-entrypoint /absolute/young-tableaux/build-apk.ysh /absolute/young-tableaux /absolute/android-sdk /absolute/android-ndk-r27c /absolute/wegert-debug.keystore
```

The scripts are canonical Grease/YSH source, not Bash wrappers. This job executed them with `dilapidated-shed/grease@f19c94c6df18cddbdc1e81463e5bd689533e3c13`'s pinned Oils-derived source `6d29702a10ea9eb72a43950554dbcd4174d07a89`, through its `bin/ysh` reference/dev entrypoint. Native Grease compilation is not claimed. Generated reference parser/modules and required libc/posix/fanos/fastfunc extensions were built with the bundled CPython 2.7.13; the optional interactive line_input extension could not build without readline headers, but noninteractive script execution passed. No other shell interpreted these scripts.

Build output: `build/young-tableaux-armeabi-v7a.apk`, signing/package/ABI reports, digest, and source SHA. The script produces a stripped ARMv7 shared library, includes only `armeabi-v7a`, aligns to 16 KiB, and verifies the stable signer before and after packaging. Fixed ZIP timestamps and metadata produce repeatable APK bytes. The checked-in `artifacts/` APK is compared with an exact-head rebuild before delivery.

## Verification and acceptance

Host tests cover parsing/limits/overflow, conjugation, cells and corners, hook tables/products/counts, known fillings and P/Q pairs, all 720 size-six permutations with independent LIS checks, every partition through size ten via independent branching counts, stable IDs, hit testing, focus/edit events, cancellation, multi-touch, scroll bounds, and unimplemented dispatch. Hosted previews use the real console projection and raster code at 576×1152; they are layout QA, not mathematical or device proof.

AddressSanitizer/UBSan run with `ASAN_OPTIONS=detect_leaks=0`; this container's ptrace/process restrictions prevent LeakSanitizer from traversing /proc. No leak-check success is claimed.

Physical MIRO A1 installation, launch, touch, scrolling, keyboard edits, and resume/recreation behavior are **PENDING**. No ADB connection is available here, and no emulator result is substituted. Next physical check: install the exact APK, launch the long surface, change lambda to `2,1` (count 2), scroll through all sections, change permutation to `3,1,4,2` and inspect P/Q, then invoke LR and verify its visible signature. Record device identity, APK digest, and actual results.
