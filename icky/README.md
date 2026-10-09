# Division glyph producer and independent acceptance

Maintained C expressions now use `÷` directly. ICK `c61e448251744a2f40ad743ebef1a027bdcd2f9d` compiles the source without normalization, through `isomorphisms/ai-ci@4ea071a96239f3a29ca6d98454feb59947d87cfe`.

There are 102 arithmetic glyph sites. Four compound assignments became explicit division of simple, stable, non-atomic local or structure lvalues; none repeats a side effect. All literal text and the separately qualified Lua gitlink remain unchanged.

Both existing Grease APK launchers and the workflow's packaging commands call `icky/Android.mk`. The fixed interface compiles all fifteen owned application C units with ICK, then NDK r27c assembles them and links them with the exact Lua runtime source and unchanged NativeActivity glue. API26, ARMv7 ARM/NEON and ARM64 payloads, Fortify2, stack protection, strict warning errors, link hardening, 16-KiB page alignment, package version and persistent signer are retained. Installed stages are restored under `.ick-stages/<abi>`, outside the packaging directory that the existing workflow recreates.

The existing modified-Lua admission remains on its independently qualified ICK515 producer and exact interpreter/library/stock-control evidence. Current native mathematics and the existing independent Python oracles select current ICK separately. The renderer's existing signed-overflow trap uses GCC's equivalent `-fsanitize-undefined-trap-on-error` alongside the same signed-integer-overflow instrumentation.

## Bounded copy equivalence

`symmetric_text` still constructs a NUL-terminated local `char result[8192]` through checked appends and rejects `length >= capacity` before writing the caller's output. The zero case copies the two bytes `"0\0"`; the successful final copy uses checked `memcpy(out,result,length+1)`. Its source is a local array and cannot alias the caller's output. The copied byte sequence and NUL, failure status and unchanged output on insufficient capacity are preserved.

The existing consolidation suite now also checks exact output bytes at the precise capacity, the NUL and an adjacent sentinel, a one-byte-short unchanged output, and the zero output at capacities one and two. Both the original HEAD algebra implementation using strcpy and the migrated implementation pass all 106,628 checks. The migrated version also passes under ASan/UBSan. The shared Fortify adapter's object-size checking remains active; no unsupported unchecked copy is substituted.

## Existing mandatory audit failures repaired

Selecting ICK for the unchanged independent audit reproduced eight prior dispatch/input-validation counterexamples and both INT_MAX renderer traps. The source fixes address their causes:

- The partition panel now actually emits its already computed removable and addable cell lists.
- Selected-cell parsing checks decimal range before conversion to int, preserving comma syntax and rejecting the wrapped `4294967297,1` input.
- Native skew validation checks both partitions before indexing or computing sizes, so an increasing row sequence cannot enter rectification.
- RSK plot extrema and differences use 64-bit integers before increment/subtraction; the ordinary integer interpolation is unchanged while INT_MAX remains representable.

The full independent audit now passes 171,228 checks with zero counterexamples, all eight compiling mathematical mutants are killed by real counterexamples, and all four existing renderer probes pass. The original host regression passes 9,924 checks and the extended consolidation suite passes 106,628 in normal and ASan/UBSan execution. Both complete Android native libraries compile and link with the same source repairs.

No oracle, expected result, warning, sanitizer or negative-control criterion was weakened. Historical receipts under `docs/audit-evidence/` remain historical; current native evidence is recorded separately here. These source/host/link checks do not establish physical-device behavior, and first-pass Functorial C completion remains separate.
