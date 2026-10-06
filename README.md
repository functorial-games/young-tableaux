# young-tableaux

Young diagrams/tableaux exploration console plus a reference collection for symmetric-group representation theory, RSK, symmetric functions, and related combinatorics.

The native Android app is an interactive tableaux laboratory. The 0.4.0 operation registry executes the ordinary non-Pauli surface: direct diagram editing, row RSK and its permutation inverse, jeu de taquin, promotion/evacuation, LR tableaux and Schur products, ordinary complex characters, exact symmetric functions, branching and paths, random generation, subsequences, reduced words, and strong Bruhat order. See the [contracts, input grammar, bounds, and verification](docs/consolidated-operations.md). The workflow builds signed ARMv7 MIRO A1 and AArch64 C67 APKs; physical acceptance is separate. The checked-in [older ARMv7 APK](artifacts/young-tableaux-armeabi-v7a.apk) is historical.

## Reference map

- [books/](books/) — books, monographs, and substantial texts
- [papers/](papers/) — foundational and useful papers/surveys
- [notes/](notes/) — public lecture notes and course material
- [software/](software/) — computational references for checking examples and experiments
- [topics/](topics/) — subject map and candidate mathematical inputs/operations
- [types/YoungTableaux/Types.idr](types/YoungTableaux/Types.idr) — broad Idris type sketch for mathematical inputs, operations, controls, and events
- [notes/first-apk-control-surface.md](notes/first-apk-control-surface.md) — first-APK exploration UI and native-window renderer plan
- [cross-links.md](cross-links.md) — links to related repositories and subjects

The bibliography favors publisher, author, university, journal, DOI/arXiv, and official software-documentation links. A link here is not an assertion that the linked work may be redistributed; do not commit copyrighted book scans merely because a copy is visible online.

## Immediate mathematical neighborhood

Young diagrams and partitions; standard and semistandard Young tableaux; hook lengths; Specht modules and irreducible representations of symmetric groups; branching; symmetric functions and Schur functions; Robinson–Schensted–Knuth; jeu de taquin; Littlewood–Richardson; plactic monoids; growth diagrams and differential posets; Schur–Weyl duality; Plancherel measure and asymptotic shape; Coxeter/noncrossing combinatorics.
