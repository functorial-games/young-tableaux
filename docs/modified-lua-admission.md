# Existing Lua runtime admission — 2026-10-08

The existing `app/assets/facts.lua` owns explanations, layout and candidate
shape edits; native C still validates and installs shape transactions. This
change refreshes that existing runtime. It does not extract additional
mathematics, rendering or Idriç semantics into Lua.

`third_party/lua` and both declared NDK producers now select
`isomorphisms/lua@87306483cec50f8c750a22dda1d0742246fad756`, tree
`6b2968928611542c8f82448f49c32ea8bc98b559`, Lua 5.5.1. The producers reject a
dirty source tree as well as the old pin. The APK workflow requires the separate
host admission job before its existing NDK build stage.

The admission uses the shared runtime owner at
`isomorphisms/flexible-pipes@5caaba31256bfb424638f8a3d4d554c56fa9a970`:
[isomorphisms/flexible-pipes PR #52, “Qualify modified Icky Lua interpreter and embedded runtime through exact ICK”](https://github.com/isomorphisms/flexible-pipes/pull/52).
That review candidate has not been promoted to the owner's default branch.
Its qualification program builds the actual interpreter and static library
with ICK `515c0f29fe6e2e96e10495fbaf25da93532e7722` from FastChat run
37589849631, artifact 11468256648. The fixed compiler and runtime digests are
checked by the owner; stock tools from PATH are not selected.

`tests/qualify-lua.grease` calls that shared qualifier and its eight real
negative controls. It then reuses the owner's C boundary fixture to embed
`tests/facts_test.lua` and `app/assets/facts.lua` with `.incbin`, load them
through `luaL_loadbufferx`, and execute the consumer assertions. It links the
same checked static runtime object and checks its digest before and after each
link. The finished executables' source sections are extracted and compared
with the unmodified source files. Dynamic dependencies must contain no second
Lua runtime. The same raw facts-test fixture linked against the same-version
stock runtime must fail in the parser; an unrelated loader error is rejected.

Host execution passed the actual interpreter and embedded symbolic fixture,
all eight shared negative controls, both consumer raw-source loads, all facts
assertions, both byte comparisons and the stock-linked consumer rejection.
The static modified runtime object SHA-256 was
`72d3e53ebfae00e83c6ac12a80b769c8c6273464dd1bfe52343dabca652fb34f`.
The facts-test and asset source digests were respectively
`2651c5f61e6e2a6903b948d1aaaf30d415eb606425fa2bc8a0595d4778ab4007`
and `c3eeb137e65a0c198c8ea8a9486c6395d1b63106300cb65a71e1746c4127dbcb`.
The unchanged native host regression also passed 9,924 checks through actual
ICK with explicit GCC-13 host headers/link support.
The unchanged consolidation suite also passed 106,612 checks. These are the
existing regression suites; their scope does not supersede the independent
44-operation audit.
The Grease host runner preserves both address/undefined sanitizer executions,
the compiling standard-result weakening mutant, insertion interception,
three incompatible C-type rejection fixtures and both Idriç sketch/specification
drift controls. Those host executions passed; the sketch checks remain textual
correspondence checks, not Idriç type-checker execution.

The CI preserves `consumer.tsv`, the shared `runtime/receipt.tsv`,
`negative/negative-controls.tsv`, source sections, executable hashes and
stdout/stderr as exact-head artifacts. To run the same admission, invoke
Grease on `tests/qualify-lua.grease` with absolute paths for the project,
that exact Flexible Pipes checkout, stock Lua source
`0b29f408433e92953cc72b1d3e06c7ac8139e439`, qualified ICK driver, ICK frontend
directory, Grease executable and a fresh output directory. Dependency expiry
or denied artifact access is a blocker; it does not select a substitute.

Whole-application readiness remains blocked. The independent
[isomorphismes/young-tableaux PR #17, “Independently audit all 44 Young Tableaux operations”](https://github.com/isomorphismes/young-tableaux/pull/17)
reports ten failures and remains separate. First-pass Functorial C completion
and the Android toolchain boundary remain unresolved. This host receipt does
not qualify the NDK-built ARM library, APK contents or physical behavior.
Android packaging and physical acceptance are **NOT_RUN** for this change.
The existing package identity, version and stable Wegert signer are preserved.

The producer manifest declares ICK for the host admission, native host
regressions and policy verifiers, and NDK 27.2.12479018 for Android compilation
and platform linking. Generic `cc` no longer selects a maintained host stage.
The NDK gap is specifically
`gap:young-tableaux-full-application-ick-qualification-not-run`: the qualified
ICK revision has current FastChat API-26/POSIX evidence, but the full Young
Tableaux Android translation and platform link were not qualified in this
change. This is missing consumer evidence, not a claimed Bionic compiler
failure. The manifest and workflow policies are required executable checks.
