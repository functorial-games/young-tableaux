# Direct Android execution boundary

Status: `BLOCKED` for the complete Android application. Direct DEX and ARM
positive controls emitted their targets; the model probes reached explicit
backend refusals.

## Intended path

An Idriç action changes a tableau model, the painter derives a fresh frame from
that model, and the presentation owner posts that frame to the current Android
surface. The host Chez run is a separate semantic and pixel observation. It does
not provide the Android event or display owner.

The core compiler available on this build host is
`dilapidated-shed/Idric@ff4d852862a3942592f8ade9afde8d409d9803be`.
The live canonical backend repository now resolves to
[`fuego-ironworks/idric-arm-thumb`](https://github.com/fuego-ironworks/idric-arm-thumb).
Its `main` head was
[`044877107e179df2ab3c8420867d71faff254f9f`](https://github.com/fuego-ironworks/idric-arm-thumb/commit/044877107e179df2ab3c8420867d71faff254f9f)
and its separate `native-arm` head was
[`0ccef59e21415585c265b79360164f7351baa1c5`](https://github.com/fuego-ironworks/idric-arm-thumb/commit/0ccef59e21415585c265b79360164f7351baa1c5)
when read on 2026-10-09. The native line uses its own declared compiler
`081b9cde0591154839fb5d80d76e5570e0436300`.

The newer cross-project DEX extension also inspected is
`isomorphisms/android-NDK@7ea62a3e3cd19f2ab04a27f72ace1ae9e8b1fe66`,
at `dex/idric/src/Backend/DEX/`. Its checked framework-call/IO work is not
silently projected onto either canonical backend branch.

## Small dynamic-update probe

`_/NativeRecursiveModel.idric` keeps the raw exported ABI within the backend's
declared `Int32` surface. The update applies a positive number of increments to
an input value. Both the step count and initial value are runtime parameters;
the result cannot be replaced by a constant sample. Nonpositive step counts
leave the value unchanged. This isolates an ordinary recursive checked helper
before adding ragged tableau storage or window effects.

The intended signature is `advance_value : Int32 → Int32 → Int32`. The helper
is pure. Its explicit `covering` annotation records that the inherited checker
does not prove termination for decreasing machine integers; positive counts
decrease by one until zero. No callback, foreign implementation, alternate
renderer, or manually encoded candidate supplies its behavior.

`_/NativeArrayBoundary.idric` separately asks whether caller-owned dynamic
storage can cross a DEX boundary, using the already declared external reference
mechanism and an array descriptor. It is a source-level boundary probe, not a
proposal to define a mathematical tableau as a platform array.

`_/NativePositiveControl.idric` is a two-parameter addition within the declared
bounded DEX subset. It must actually produce fresh DEX before a refusal can be
attributed to an unsupported program form rather than a wholly unusable driver.

The independently maintained `native-arm` line is checked with
`_/NativeARMPositive.idric`, a deliberately Float32 scalar addition, and
`_/NativeARMModel.idric`, the same whole-number result needed by tableau entries
and revision counts. This distinguishes the native scalar ABI from DEX and
does not substitute float values for exact tableau entries.

## Reusing the compiler without rebuilding it

The existing core compiler and its installed base/prelude/API modules are used
directly. Earlier local attempts to optimize a whole custom DEX driver with
Chez ended with return code 137 and empty `.so` files. Those files are not
executables. The current compiler's documented `--exec` path produced its own
complete 9,703,205-byte Chez driver source. Running it was killed, including a
second attempt using Chez's documented `--optimize-level 0`. The environment
reported memory pressure and out-of-memory kills. That extension-driver attempt
remains `BLOCKED`; it supplies no DEX execution evidence.

An already compiled driver was then found for the canonical DEX implementation.
All seven generic backend modules and `backend.ipkg` are byte-identical between
live canonical `main` and the clean `android-NDK@65769fd68a30a26e0dacd2d0f9ba182850857e86`
source associated with that local driver. Their Git blob identities are
retained in [`_/native-evidence/dex-material-blobs.tsv`](_/native-evidence/dex-material-blobs.tsv).
The driver reports compiler `0.8.0-ff4d85286`; its payload SHA-256 is
`e03122427c52e0d893cac4184583b5724aadd8ac063ed164e8216e75ee40ba0a`.
No new native-code compiler build was needed for the actual probes.

Commands, source identity, retained diagnostics, generated artifacts, and their
absence after a refused compile are recorded under `_/native-evidence/`.

## Result

| Program and boundary | Actual result |
| --- | --- |
| `NativePositiveControl.idric` through current DEX material implementation | `PASS`: fresh 436-byte DEX plus checked ANF, typed plan, and readable target listing |
| Independent Android SDK 35.0.0 `dexdump` parsing | `PASS`: DEX 035, `next_frame_value(II)I`, three registers and an integer addition |
| `NativeRecursiveModel.idric` core source check | `PASS` |
| Same recursive program through DEX | `REFUSED`: `Unsupported checked named call in DEX checked slice: NativeRecursiveModel.advance_value`; no target artifact |
| `NativeArrayBoundary.idric` core source check | `PASS` |
| Same array/IO boundary through DEX | `REFUSED`: exported source ABI accepts explicit `Int32` and `Text` only; no target artifact |
| `NativeARMPositive.idric` through current `native-arm` | `PASS`: fresh Float32 addition assembly |
| `NativeARMModel.idric` through current `native-arm` | `REFUSED`: `result must be RendererPrimitives.Float32, not Word32`; no target artifact |
| Full application DEX/native artifact, ART execution, posted Android frames, and physical phone | `NOT_RUN` |

The DEX positive artifact SHA-256 is
`afa9a4a150d3c6ebec2d44d1912dd93d4cf26b08153380809775900edcd9dc6a`.
Its raw bytes and adjacent compiler products are retained beside the receipt.
The ARM control is assembly emission only; no object, linked Android library,
or ARM execution is claimed.

Both DEX refusals printed an error while returning process status zero. The
record therefore requires the actual refusal and an empty fresh output
directory. A zero status is not a passing compiler result. The host qualifier
similarly requires clean diagnostics, a new nonempty executable payload, an
executed complete assertion marker, and stable source hashes.

## Smallest next backend capability

For canonical DEX, lower an ordinary checked recursive call or its equivalent
loop, preserving runtime parameters, and execute `advance_frame_value` on ART
for zero, positive, and nonpositive step counts. It must produce the actual
input-dependent result. This is the first observed call boundary; supporting it
does not by itself add ragged tableau storage, callbacks, or window posting.

The inspected newer framework-call extension has a more explicit but still
bounded helper-specialization stage. Its source rejects a helper already on the
specialization stack with `Recursive checked helper is not in the DEX
specialization slice`. That is a source-review finding, distinct from the
canonical driver's executed refusal.

For native ARM, first preserve whole-number result values through the existing
ABI and emit the same `next_frame_value` probe with an integer result. The
present Float32 scalar result requirement cannot represent exact tableau state.
No existing rasterizer should be used to conceal that boundary.

The application acceptance remains the same after those narrow fixes: compile
the maintained model and painter, change RSK/JDT state from an actual event,
post the resulting frame, and observe the changed tableau pixels. Pixel buffers,
successful post acknowledgments, Android runtime execution, and physical-device
behavior remain separately reported.
