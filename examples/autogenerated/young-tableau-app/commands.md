# Young Tableau command grammar

The Idriç host runner accepts all 44 ordinary laboratory operations with explicit
arguments. It starts a fresh model, applies the supplied command strings from
left to right, and writes one PPM image of the final model. A successful action
updates the model and derives its displayed objects from the returned result.

The implementation is in [Young/Commands.idric](Young/Commands.idric). Its entry
point is `apply_command : Text → Model → Either Text Model`. The mathematical
request and result types live in [Young/Laboratory.idric](Young/Laboratory.idric).

## Running a sequence

The host qualifier writes the runtime entry point at
`OUTPUT/exec/young-idric`, where `OUTPUT` is the absolute output directory passed
to [the qualifier](_/qualify-host.grease). The surrounding qualification guide
describes the build invocation.

Replace the two `/ABS/...` paths below with the actual qualification directory
and an image path whose parent directory already exists. This is one external
command in Grease; each quoted action is a separate argument to the application.

```grease
/ABS/qualification-output/exec/young-idric --render /ABS/images/rsk.ppm 'word 3,1,1' 'next' 'next'
```

The example shows the first two insertions of the entered word. `RSKWord 3,1,1`
computes and displays the completed P and Q in one action. `word 3,1,1` loads that
word at the beginning, so subsequent `next` actions expose its individual
insertions.

The runner syntax is `young-idric --render IMAGE.ppm COMMAND ...`. The commands
below are the quoted argument contents, not separate executable names. A failed
command stops this invocation before it writes the requested final image.

## Lists, rows, cells and skew tableaux

Scientific operation names are case sensitive. Whitespace separates arguments;
keep each list, row collection and symmetric function within a single argument
field. The parser rejects missing and extra arguments.

| Input | Grammar | Example |
|---|---|---|
| Integer | One exact signed integer | `-3` |
| Size | One nonnegative integer | `4` |
| Word or alphabet | Comma-separated integers | `3,1,1,2` |
| Empty list | `[]` | `[]` |
| Partition | Positive row lengths in nonincreasing order | `3,2,1` |
| Tableau | Commas within rows; semicolons between rows | `1,2,4;3,5` |
| Cell | Two positive, one-based coordinates: `row,column` | `2,3` |
| Matrix | Commas within rows; semicolons between rows; nonnegative entries | `0,2;1,0` |
| Biword | Separate upper-word and lower-word arguments | `1,1,2 2,2,1` |
| Skew tableau | Separate outer-partition, inner-partition and visible-row arguments | `3,2,1 1 1,3;2,5;4` |

Optional surrounding brackets work for individual lists, such as `[3,2,1]` or
`[1,2];[3]`. Use `[]` for an empty partition, word, matrix or tableau. Empty
interior fields and trailing separators are errors.

In a skew tableau, the row lists contain exactly the visible entries after the
inner boxes are removed. Do not insert zero placeholders for inner boxes. An
empty visible row is written `[]` within the semicolon-separated row collection.
`JeuDeTaquinSlide` adds one further cell argument after the three skew arguments.

A permutation contains each integer from 1 through its size exactly once. A word
may repeat positive letters. A biword has equal-length rows and ordered positive
biletters. Matrices have equal-length rows. Inverse RSK accepts standard P and Q
of the same shape and returns a permutation. Promotion and evacuation also
require standard tableaux.

## Validation codes

`ValidateTableau RULE ORDER ROWS` evaluates the selected property. A property
that does not hold produces a displayed `False`; malformed shape or invalid
codes produce an error.

| Rule code | Property |
|---|---|
| `0` | Arbitrary filling of the checked shape |
| `1` | Row standard: labels 1 through the size, ordered strictly along rows |
| `2` | Column standard: labels 1 through the size, ordered strictly down columns |
| `3` | Standard: labels 1 through the size, ordered strictly in rows and columns |
| `4` | Positive semistandard: weak rows and strict columns |

| Order code | Direction |
|---|---|
| `0` | Increasing |
| `1` | Decreasing |

For example, `ValidateTableau 3 1 4,3;2,1` checks a decreasing standard tableau.
The ordering choice applies to both rows and columns as required by the selected
rule. Arbitrary fillings may contain signed entries; the semistandard alphabet
is positive.

## Exact symmetric functions

A function has the grammar
`BASIS;COEFFICIENT;PARTITION;COEFFICIENT;PARTITION...`. A coefficient is an integer
or `NUMERATOR,DENOMINATOR`, with a positive denominator. Coefficients use exact
rational arithmetic. Both ASCII minus and `−` are accepted in coefficients.

| Basis code | Basis |
|---|---|
| `0` | Schur |
| `1` | Power sum |
| `2` | Monomial |
| `3` | Complete homogeneous |
| `4` | Elementary |

Examples:

- `0;1;2,1` means the Schur function indexed by `[2,1]`.
- `1;1,2;2;1,2;1,1` means one half of `p[2]` plus one half of `p[1,1]`.
- `0` is the zero Schur sum.
- `0;1;[]` is the constant one.
- `0;−2,3;[]` is the constant minus two thirds.

`ChangeBasis FUNCTION DESTINATION_CODE` converts the complete function.
`Specialize FUNCTION ALPHABET` evaluates it on a finite integer alphabet;
negative letters and the empty alphabet are allowed. `Plethysm OUTER INNER`
uses this argument order and treats rational coefficients as fixed constants
under the power-sum substitutions.

## All 44 scientific commands

Every command in this table is an executable argument to `--render`. The exact
same strings appear in the independently maintained command fixtures in
[CommandsChecks.idric](CommandsChecks.idric). Each is executed and compared with
its corresponding typed request and full resulting model.

| Legacy operation | Executable sample command |
|---|---|
| DisplayPartition | `DisplayPartition 3,2` |
| ConjugatePartition | `ConjugatePartition 3,2` |
| ListCells | `ListCells 3,2` |
| DisplayTableau | `DisplayTableau 1,2,4;3,5` |
| ValidateTableau | `ValidateTableau 3 0 1,2,4;3,5` |
| StandardizeTableau | `StandardizeTableau 1,1,3;2,3` |
| InsertLetter | `InsertLetter 1,3;2,4 2` |
| ReverseInsert | `ReverseInsert 1,2;3,4 2,2` |
| TransposeTableau | `TransposeTableau 1,2,4;3,5` |
| ComputeHookLengths | `ComputeHookLengths 3,2` |
| ComputeHookProduct | `ComputeHookProduct 3,2` |
| CountStandardTableaux | `CountStandardTableaux 3,2` |
| FindCorners | `FindCorners 3,2` |
| FindAddableCells | `FindAddableCells 3,2` |
| FindRemovableCells | `FindRemovableCells 3,2` |
| AddCell | `AddCell 3,2 3,1` |
| RemoveCell | `RemoveCell 3,2 1,3` |
| RSKPermutation | `RSKPermutation 3,1,4,2` |
| RSKWord | `RSKWord 2,1,2` |
| RSKBiword | `RSKBiword 1,1,2 2,2,1` |
| RSKMatrix | `RSKMatrix 0,2;1,0` |
| InverseRSK | `InverseRSK 1,2;3,4 1,3;2,4` |
| JeuDeTaquinSlide | `JeuDeTaquinSlide 3,2,1 1 1,3;2,5;4 1,1` |
| Rectify | `Rectify 3,2,1 1 1,3;2,5;4` |
| Promote | `Promote 1,2;3,4` |
| Evacuate | `Evacuate 1,2;3,4` |
| LittlewoodRichardsonCoefficient | `LittlewoodRichardsonCoefficient 2,1 2,1 3,2,1` |
| EnumerateLRTableaux | `EnumerateLRTableaux 2,1 2,1 3,2,1` |
| MultiplySchurFunctions | `MultiplySchurFunctions 1 1` |
| CharacterValue | `CharacterValue 2,1 2,3,1` |
| RepresentationDimension | `RepresentationDimension 3,2` |
| ChangeBasis | `ChangeBasis 0;1;2 1` |
| Specialize | `Specialize 0;1;2,1 1,2` |
| Plethysm | `Plethysm 0;1;2 0;1;2` |
| BranchUp | `BranchUp 3,2` |
| BranchDown | `BranchDown 3,2` |
| EnumerateYoungGraphPaths | `EnumerateYoungGraphPaths [] 2,1` |
| GenerateRandomPermutation | `GenerateRandomPermutation 4` |
| GenerateRandomStandardTableau | `GenerateRandomStandardTableau 2,1` |
| SamplePlancherelPartition | `SamplePlancherelPartition 4` |
| ComputeLongestIncreasingSubsequence | `ComputeLongestIncreasingSubsequence 3,1,4,2` |
| ComputeLongestDecreasingSubsequence | `ComputeLongestDecreasingSubsequence 3,1,4,2` |
| CoxeterReducedWord | `CoxeterReducedWord 3,1,4,2` |
| BruhatRelations | `BruhatRelations 1,2,3 3,2,1` |

The three Littlewood–Richardson arguments are inner partition, content partition
and outer partition, in that order. The corresponding coefficient counts
fillings of outer minus inner with the given content. `CharacterValue` takes a
partition and a one-line permutation. `BruhatRelations` asks whether its first
permutation is below or equal to its second in the strong Bruhat order.

## State, random generation and result pages

The initial random seed is zero. `seed INTEGER` explicitly sets the 64-bit
SplitMix64 state, accepting integers from zero through `18446744073709551615`.
Successive random commands within the same invocation advance that shared
stream. They do not reset to a sample seed. Setting the same seed again restarts
the same sequence. A failed request does not expose an advanced generator state.

`result-next` and `result-prev` browse the current laboratory result. The model
retains the complete result while a page shows at most ten text rows and two
diagrams. Running another scientific operation selects its first result page.
An attempt to pass either endpoint, or to browse before any result exists,
returns an error.

Basic actions remain available through the same command entry point:

| Action | Meaning |
|---|---|
| `word WORD` | Load a word at insertion step zero |
| `permutation WORD` | Load a checked permutation at insertion step zero |
| `biword UPPER LOWER` | Load an ordered biword at insertion step zero |
| `matrix ROWS` | Load a nonnegative matrix at insertion step zero |
| `next`, `prev`, `start`, `end` | Navigate the current RSK input |
| `partition PARTITION` | Set the reference partition |
| `tableau ROWS` | Set the selected tableau |
| `skew OUTER INNER ROWS` | Load a skew tableau |
| `corner ROW COLUMN` | Select a one-based inner corner using two integer arguments |
| `step` | Start or advance one jeu de taquin move |
| `slide` | Complete the selected or active jeu slide |
| `rectify` | Rectify the current skew tableau |
| `reset` | Restore the loaded skew tableau |

The basic `corner` action retains its existing two-integer grammar. Scientific
operations such as `AddCell`, `ReverseInsert` and `JeuDeTaquinSlide` use a single
`row,column` cell argument.

## Bounds and errors

The application admits shapes of at most 64 rows, 64 columns and 256 cells, and
RSK inputs of at most 64 letters. Character and symmetric-function calculations
are bounded to degree 8; LR enumeration admits at most 8 visible skew cells and
retains at most 32 tableaux. Symmetric functions have at most 32 terms. Graph
enumeration retains at most 128 paths, each with at most 16 edges. Reduced words
have at most 256 adjacent generators. Uniform tableau sampling requires its
corner-weight sum to fit a single 64-bit generator draw.

Malformed arguments, unsupported sizes, zero denominators, invalid corners,
and exhausted result limits return errors. Oversized enumerations are not
displayed as truncated complete answers. The command checks cover every
scientific operation, strict argument counts, sequential navigation, reproducible
random state, pagination boundaries and failure preservation.
