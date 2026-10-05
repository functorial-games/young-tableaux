# First APK: exploration control surface

The first APK should **not** assume that the central interaction is "tap a box to add it" or any other single tableau manipulation.

The user is still learning what the major constructions do. The first APK should therefore make the mathematical input/output space visible rather than hide it behind one interaction metaphor.

## v0.1 shape

Build a long, vertically scrollable control surface divided by horizontal rules. Each section exposes relevant inputs, conventions, operations, and an output region.

Candidate sections:

- partition / Young diagram
- tableau
- hooks, corners, addable/removable cells
- RSK
- jeu de taquin
- Littlewood–Richardson
- symmetric group
- representation theory
- symmetric functions
- Young graph / branching
- random and asymptotic experiments
- type-A / Coxeter bridge
- global conventions

It is acceptable, and useful, for controls for unimplemented operations to exist. Activating one can report the expected typed input and output plus `NOT IMPLEMENTED`.

Example:

```text
LITTLEWOOD-RICHARDSON
lambda      [ ... ]
mu          [ ... ]
nu          [ ... ]
content     [ ... ]

[ LR coefficient ]
[ enumerate LR tableaux ]
[ Schur product ]

OUTPUT
...
```

The point of v0.1 is exploration: make it easy to see what data an operation consumes and what kind of result it produces.

## UI vocabulary

Keep the low-level UI small even though the semantic control inventory is large. Initial widget behaviors can be limited to:

- label
- horizontal separator
- text field
- integer field / +/- stepper
- choice selector
- action button
- output text
- output diagram
- vertical scrolling

A control carries a semantic purpose separately from its presentation. See `types/YoungTableaux/Types.idr`.

## Rendering

Do **not** introduce Sokol for the first APK merely to get pixels on the screen.

Reuse the existing native software renderer from:

`Ashtray-Archer/utilities-android-phone-user/accelerometer/app/src/main/c/native_main.c`

That code already provides:

- `android.app.NativeActivity`
- `ANativeWindow`
- `ANativeWindow_lock` / `ANativeWindow_unlockAndPost`
- RGBA8888 software rendering
- pixel writes
- filled rectangles
- antialiased 5x7 glyph rendering
- text rendering and text measurement
- Android window/focus lifecycle
- existing APK packaging and multi-ABI build precedent

This is not direct `/dev/fb0` access. It is software rendering into Android's locked native-window buffer, which is sufficient for this application.

The missing generic pieces are primarily:

- touch-event handling
- hit testing from coordinates to `ControlId`
- text-entry plumbing
- vertical scrolling
- reusable control layout

The intended path is:

```text
Android input event
      |
      v
hit test / text input / scroll
      |
      v
typed ControlId + semantic action
      |
      v
mathematical state transition
      |
      v
output projection
      |
      v
rectangles + glyphs + blocks/dots
      |
      v
ANativeWindow_Buffer
```

## Output

Do not over-specify the output representation yet.

At the mathematical-display boundary, Young diagrams/tableaux can often be represented by very small primitives such as blocks or dots. Other operations may need text, lists, coefficients, arrows, highlights, or paired tableaux.

The renderer should remain ignorant of Littlewood–Richardson, RSK, Specht modules, etc. It should only receive simple drawing/layout requests.

## Architectural rule

Keep these distinct:

1. mathematical input
2. mathematical operation
3. interaction event
4. UI control state
5. mathematical result
6. render projection
7. Android/native-window drawing

The Idris sketch may enumerate far more cases than v0.1 implements. That is intentional.
