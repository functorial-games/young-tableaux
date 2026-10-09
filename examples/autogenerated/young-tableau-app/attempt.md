# Program attempt: Young Tableau in Idriç

Status: `PARTIAL` — Idriç host implementation; complete Android application blocked.

## Purpose

The user reports that RSK and jeu de taquin do not visibly change the picture.
This branch attempts an Idriç implementation of the existing mathematical
laboratory, including its state transitions and picture production. It starts
from the latest native tableau work at
`7728a9c8518337a405e09b53b8f2372d35bc08c4`; the native application remains available
as an independently implemented reference while this branch is incomplete.

## Intended behavior

An input action changes one authoritative mathematical state. Its displayed
tableaux and any affected plots are derived from that state. RSK exposes P, Q,
and its current insertion path. Jeu de taquin exposes the entries and the hole,
including intermediate moves, completed slides, rectification, and reset.
Previous/next and failed inputs must have explicit, testable behavior.

The fixed outer-partition Wegert picture depends on its partition, not on an
unrelated hole movement. A current-tableau shape picture may change when its
own partition changes. The Idriç implementation must preserve this distinction.

## Type-system sketch

### Values and domains

- A partition is a finite nonincreasing list of positive row lengths.
- Cells use the existing one-based row and column convention.
- A tableau associates integer entries with a partition's cells; an admitted
  semistandard tableau has positive entries, weak rows, and strict columns.
- A skew tableau has an outer partition, a contained inner partition, and
  exactly the visible entries. An in-progress slide has a separate hole state.
- RSK keeps insertion and recording tableaux of the same shape, the input
  word or ordered biword, and the current insertion count and bump path.
- User actions, model revisions, source-derived drawing commands, pixels,
  surface availability, and successful presentation are separate values.

### Types and signatures

The mathematical implementation exposes checked constructors and explicit
`Either Text result` errors. It uses `Number` for nonnegative whole numbers,
`Integer` for signed entries and exact arithmetic, and `Text` for decoded text.
`List` expresses ragged rows; no generic vector type defines a tableau.

Central operations are partition validation/conjugation/corners/hooks,
tableau validation/insertion/reverse insertion, all four row-RSK inputs,
inverse permutation RSK, hole movement/slide/rectification, promotion and
evacuation, and the existing algebraic, graph, permutation and random surface.

The application transition has the shape
`apply_action : user_action → Model → Either Text Model`.
Painting has the source-indexed shape
`paint : (source : Model) → Frame source`.
Frame construction belongs to the painter; consumers receive only its
read-only pixels and source identity. A presentation acknowledgment must refer
to the complete pending source model, actual window identity, and surface generation.

### Actions and effects

Mathematics, action reduction, layout, and rasterization are pure. Reading an
event, writing a frame, and acknowledging successful window presentation are
effects. A failed post never advances the presented revision. Window loss does
not discard the mathematical state; a new window requests its latest frame.

### Laws, invariants, and errors

- Inserting a letter bumps the first strictly greater row entry.
- Equal jeu neighbors choose the entry below, preserving strict columns.
- No-op navigation at an endpoint is explicit; no changing label can stand in
  for changed tableau pixels.
- Invalid shape/input/corner/action preserves the previous model.
- Known nontrivial RSK and jeu actions must change the relevant board pixels.
- Rendering stale state and acknowledging failed/stale posts must be rejected.
- Missing backend facilities are errors, not permission to change language.

### Runtime and backend boundary

The available Idriç host compiler is
`dilapidated-shed/Idric@ff4d852862a3942592f8ade9afde8d409d9803be`, whose existing
Chez backend can check and execute the program on this build host. That is
host semantic and pixel evidence only. Android requires an actual maintained
Idriç backend to lower this program and provide event/pixel/presentation effects.
The direct DEX and ARM paths must be investigated and attempted honestly.
No RefC, generated C, Java, Gradle, or hand-written replacement is admitted.

## Idriç attempt

Source: the `.idric` modules in this directory. Build machinery lives under `_/`.

Application source:
locally qualified commit `1e8e17def5f8cf7f4c7c489b4ba4866ddae6fe7a`, published
through the connected GitHub app as
[`1f691ba9b91e2f485a4c4b6feb0f35c857cca843`](https://github.com/isomorphismes/young-tableaux/commit/1f691ba9b91e2f485a4c4b6feb0f35c857cca843)
on the new `Idriç` branch. Both source commits have the exact same Git tree;
the qualification's source hashes remain unchanged.
Compiler:
`dilapidated-shed/Idric@ff4d852862a3942592f8ade9afde8d409d9803be`.
Host backend: the existing Chez 10.4.1 installation.

The reproducible entry point is `_/qualify-host.grease`, invoked by the actual
Grease executable with four absolute arguments: the Young repository, the
Idriç repository, the Chez executable, and a new output directory. The exact
executed compiler and application commands are retained with the verification
receipts. [README.md](README.md) gives the portable invocation and
[commands.md](commands.md) gives all 44 executable operation examples.

## Result or failure

The 44-operation registry now has typed requests, dependent result types, real
Idriç mathematical implementations, and projections of the returned data.
Application actions install those results into one model. RSK begins at zero
insertions; jeu actions preserve their intermediate entries and active hole.
Both the main diagrams and the bounded, complete laboratory result pages are
painted from that model.

The host sink writes the production painter's actual PPM pixels, reads back the
file, compares it byte-for-byte, and acknowledges only a matching frame. Checks
follow pointer input through the reducer into cropped tableau pixels; they also
exercise failed posts, stale source and window identities, lifecycle changes,
random-state advancement, invalid input, and result paging. Compile-negative
fixtures try to relabel a stale frame and construct an arbitrary frame. Runtime
mutants remove painting or prevent model updates and must fail their specific
behavioral checks. The exact counts, outcomes, manifests, and limitations are
in [verification.md](verification.md).

Android target probes emitted real DEX and ARM positive controls before reaching
explicit refusals for the required recursive/storage or integer-result forms.
There is no complete Idriç Android artifact, ART execution, native-window post,
or physical-device observation from this attempt. The existing C application
has not been substituted for those missing stages.

## Fallback

None.

## Idriç language work exposed

First observed DEX blocker: lowering an ordinary checked recursive helper with
runtime inputs. The current implementation prints
`Unsupported checked named call in DEX checked slice` and emits no target for
that probe. A separate dynamic array/IO probe is refused by the exported ABI.
The independently maintained native ARM branch emits a Float32 control but
refuses the whole-number result needed by the model. See
[native-boundary.md](native-boundary.md) for pins, controls, and diagnostics.

Smallest next DEX capability: preserve and lower the checked recursive call or
its equivalent loop, then execute the input-dependent probe on ART. For native
ARM, preserve whole-number results through its existing ABI. These first fixes
still leave dynamic tableau storage, Android events, and real window effects
to implement and verify; they do not by themselves complete the application.

Acceptance test: the same maintained tableau program must pass through the
target backend and turn real actions into successfully posted frames.

## Evidence boundary

The branch is a working host rewrite attempt with an explicit Android boundary.
No phone behavior or completed native replacement follows from host arithmetic,
bitmap output, source-indexed types, or emitted positive-control target bytes.
