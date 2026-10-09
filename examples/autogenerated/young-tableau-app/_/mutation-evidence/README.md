# Compiled runtime mutations

Both mutations ran after the positive 2,383-assertion host qualification on
2026-10-09. The maintained `.idric` sources still match application commit
`1e8e17def5f8cf7f4c7c489b4ba4866ddae6fe7a` and the positive source manifest.
The isolated source copies, their fresh builds, and the generated executables
remain under `/workspace/scratch/df6a5bad82b3/idric-mutation-qualification-01`.

Each mutant compiled successfully through the same Idriç/Chez toolchain.
Each executable then returned failure and emitted its exact intended assertion;
neither emitted `YOUNG_IDRIC_PASS`. Compiler errors and unrelated runtime or
image-file errors cannot satisfy this gate.

| Mutation | Required observed assertion |
| --- | --- |
| Painter discards the model-derived scene | `RSK input-to-pixels changes P pane` |
| Successful actions return the previous model | `basic actions preserve sequential RSK input and navigation` |

The painter mutation produced seven pixel assertion failures, including the
P and Q tableaux, both jeu moves, the changed jeu shape plot, changed result
data, and changed pagination diagrams. The model mutation also failed all
44 laboratory command-installation assertions. The complete outputs, exact
one-line source diffs, isolated source manifests, fresh checked-module hashes,
and executable hashes are retained here.

The initial launch attempt stopped in Grease argument validation before any
mutation output directory or source copy was made. Grease treats `+` as a
numeric operator; the guard now uses ordinary string interpolation.
`initial-grease-refusal.txt` preserves that observed startup error, while
`qualification-runner-sha256.txt` identifies the corrected launcher that
actually completed both runtime mutations. No `.idric` source changed for
this launcher correction.

`source-repository.sha256` is the unchanged positive source manifest with
repository-relative paths. `original-before.txt` and `original-after.txt`
record that the maintained source stayed unchanged. Android and physical
device execution remain `NOT_RUN`.
