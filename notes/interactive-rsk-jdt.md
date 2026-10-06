# Interactive RSK and jeu de taquin

This note pins the conventions used by the native app so that the animation and the mathematical core are the same algorithm.

## RSK

The implemented insertion is ordinary **row insertion**.

- Read a permutation or word from left to right.
- In the current row, replace the first entry that is **strictly greater** than the inserted value.
- Insert the displaced entry into the next row.
- If no entry is strictly greater, append at the end of the row.
- Put the insertion step number in the newly created cell of the recording tableau Q.

For permutations, P and Q are standard. For positive-integer words with repeated letters, P is semistandard (weak rows, strict columns) and Q is standard.

The interactive controls expose START, PREV, NEXT, and END. Highlighted cells of P are exactly the bump path for the most recent insertion. The final result is produced by the same traced insertion routine rather than by a second implementation.

Regression examples include the permutation `3,1,4,2` and the repeated-letter word `2,1,2,1`.

## Skew-tableau input

The top-level lambda field is the outer partition. The jeu de taquin section supplies the inner partition mu.

The skew-tableau field contains only the visible cells of lambda/mu, row by row. Inner cells are not represented by zeroes or dummy values.

Example:

- lambda = `3,2,1`
- mu = `1`
- skew rows = `1,3;2,5;4`

The first visible row therefore begins in column 2.

## Forward jeu de taquin

STEP uses the selected cell as an inner corner when no slide is active.

1. If only the right neighbor exists, move it into the hole.
2. If only the lower neighbor exists, move it into the hole.
3. If both exist, move the smaller entry.
4. If the entries are equal, move the **lower** entry. This is the semistandard convention for weak rows and strict columns.
5. When neither neighbor exists, the hole is an outer corner; remove that outer cell and finish the slide.

SLIDE finishes the selected/current slide. RECTIFY finishes an active slide and then repeatedly slides removable inner corners until mu is empty. RESET reparses the fields.

The core accepts standard and semistandard skew tableaux with positive entries. An intermediate slide deliberately contains one visible hole; semistandard validation is applied again when the slide finishes.

## Sources and cross-checks

The repository bibliography supplies the mathematical provenance: Fulton, Sagan, Stanley, Robinson, Schensted, Knuth, Schützenberger, Fomin, and the Krattenthaler notes connecting RSK, jeu de taquin, and growth diagrams. Sage remains the intended external oracle for additional small-case regression vectors.

The host tests check known permutation RSK pairs, every permutation of S_6 for standardness and the first-row/LIS property, repeated-letter word insertion, bump-path coordinates, standard skew slides and rectification, and the equal-neighbor semistandard tie case.
