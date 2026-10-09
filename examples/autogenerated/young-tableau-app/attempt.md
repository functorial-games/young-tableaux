# Program attempt: Young Tableau in Idriç

Status: `NOT_RUN`

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
`apply_action : Action → Model → Either Text Model`.
Painting has the source-indexed shape
`paint : (source : Model) → Frame source`.
Frame construction belongs to the painter; consumers receive only its
read-only pixels and source identity. A presentation acknowledgment must refer
to the actual pending source revision and surface generation.

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

Revision and compiler: see the final evidence record.

Command: to be recorded after the first actual run.

## Result or failure

No result yet. Type checking, host execution, pixels, target lowering, packaging,
and physical behavior will be reported independently.

## Fallback

None.

## Idriç language work exposed

First blocking capability: to be established by a real compilation attempt.

Smallest plausible language change: to be determined from that failure.

Acceptance test: the same maintained tableau program must pass through the
target backend and turn real actions into successfully posted frames.

## Evidence boundary

No phone behavior or completed whole-application replacement is claimed by
the presence of this attempt record.
