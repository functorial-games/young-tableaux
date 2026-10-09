# Maintained-source host qualification

The actual Grease launcher completed successfully on 2026-10-09 against
application source commit `1e8e17def5f8cf7f4c7c489b4ba4866ddae6fe7a`.
The Idriç compiler was `ff4d852862a3942592f8ade9afde8d409d9803be`, using its
Chez host backend. `run.stdout` records 2,383 passing assertions and 11 image
files written and read back through the host image sink.

`source-before.sha256` and `source-after.sha256` retain the exact original
absolute manifests. They matched. `source-repository.sha256` changes only the
path prefix to make the same 24 source hashes checkable from the repository
root with `sha256sum -c`; `repository-source-check.txt` records that check.
The compiler payload, generated application payload, checked module and image
hashes retain their original paths and bytes. Large executable payloads,
checked modules and PPM files remain in the recorded scratch output directory.

The two `negative-types` directories retain the intended type diagnostics and
empty target-TTC listings. A compiler return status alone is never accepted as
successful checking or as an expected refusal.

`qualification-runner-sha256.txt` was captured before mutation qualification.
Its mutant-launcher entry is the initial version whose string-concatenation
guard was subsequently corrected. The actually executed mutation launcher's
hash and that initial Grease refusal are recorded in `../mutation-evidence/`.

This receipt observes Idriç host mathematics, state changes, pixels, and the
host image sink. Android lowering, Android window posting, and physical-device
execution are explicitly `NOT_RUN`.
