#!/usr/bin/env python3
"""Execute native JDT canvas contract and kill four silent-failure mutations.

Use an explicitly chosen host-capable ICK/NDK compiler; do not silently fall
back to system cc. The checks render the real painter, not metadata or labels.
"""
import argparse
import hashlib
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
CORE = sorted((ROOT / "app/core").glob("*.c"))
NATIVE = [ROOT / path for path in (
    "app/ui/controls.c", "app/console/nearby.c", "app/render/raster.c")]
SOURCES = ("app/console/console.c", "app/render/paint.c")
MUTATIONS = {
    "no-step": ("app/console/console.c",
                "JeuStepResult result=jeu_step(&c->jeu);",
                "JeuStepResult result=JEU_INVALID;"),
    "no-diagram": ("app/render/paint.c",
                   "} else if(c->kind==DIAGRAM) {",
                   "} else if(c->kind==DIAGRAM && 0) {"),
    "stale-wegert": ("app/console/console.c",
                     "c->diagram.addition_count=0; refresh_partition(c); refresh_tableau(c); refresh_jeu(c);",
                     "c->diagram.addition_count=0; refresh_tableau(c); refresh_jeu(c);"),
    "stale-rectified-plot": ("app/console/console.c",
                             "project_wegert_shape(&c->jeu_wegert,&c->jeu.filling.shape.outer);",
                             "if (!c->jeu_wegert.valid) project_wegert_shape(&c->jeu_wegert,&c->jeu.filling.shape.outer);"),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def execute(compiler, flags, directory, label, overrides):
    output = directory / label
    sources = [ROOT / "tests/jeu_visual_contract.c", *CORE, *NATIVE]
    sources += [overrides.get(path, ROOT / path) for path in SOURCES]
    cmd = [compiler, *flags, "-std=c17", "-O2", "-Wall", "-Wextra",
           "-Werror", "-Wpedantic", "-Wshadow"]
    cmd += [f"-I{ROOT / path}" for path in (
        "app/core", "app/ui", "app/console", "app/render")]
    cmd += [*(str(p) for p in sources), "-lm", "-o", str(output)]
    subprocess.run(cmd, check=True, cwd=ROOT)
    completed = subprocess.run([str(output)], cwd=ROOT,
                               text=True, capture_output=True)
    (directory / (label + ".stdout")).write_text(completed.stdout)
    (directory / (label + ".stderr")).write_text(completed.stderr)
    return completed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--compiler-flag", action="append", default=[])
    parser.add_argument("--output", type=pathlib.Path,
                        default=ROOT / "build/jdt-visual-evidence")
    args = parser.parse_args()
    directory = args.output.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    compiler = pathlib.Path(args.compiler).resolve()
    if not compiler.is_file():
        raise SystemExit("FAIL: the specified compiler does not exist")

    result = execute(str(compiler), args.compiler_flag, directory, "actual", {})
    if result.returncode != 0 or "JDT_VISUAL PASS:" not in result.stdout:
        print(result.stderr or result.stdout, file=sys.stderr)
        raise SystemExit("FAIL: real JDT state-to-pixel contract")

    receipts = ["case\tresult\texit_code\tsha256\n"]
    receipts.append(f"actual\tPASS\t{result.returncode}\t"
                    f"{digest(directory / 'actual')}\n")
    print(result.stdout.strip(), flush=True)

    for name, (source, original, replacement) in MUTATIONS.items():
        src = (ROOT / source).read_text()
        if src.count(original) != 1:
            raise SystemExit(f"FAIL: {name} source mutation not unique")
        mutant = directory / (name + ".c")
        mutant.write_text(src.replace(original, replacement))
        result = execute(str(compiler), args.compiler_flag, directory, name,
                         {source: mutant})
        if result.returncode == 0 or "JDT_VISUAL_FAIL" not in result.stderr:
            raise SystemExit(f"FAIL: {name} mutant escaped detection")
        receipts.append(f"{name}\tREJECTED\t{result.returncode}\t"
                        f"{digest(directory / name)}\n")
        print(f"MUTANT_REJECTED {name}: {result.stderr.strip()}", flush=True)

    (directory / "receipt.tsv").write_text("".join(receipts))
    print("JDT_VISUAL_ALL PASS: real painter, real transitions, four mutations")


if __name__ == "__main__":
    main()
