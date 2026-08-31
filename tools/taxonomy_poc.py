#!/usr/bin/env python3
"""Corpus-taxonomy PoC: run m2c on each function's target asm, compile
the output with the project toolchain, and classify the result.

Buckets: M2C_FAIL, COMPILE_FAIL, STRUCTURAL, REGALLOC_ONLY, INSN_MATCH.

Single function:
    .venv/bin/python3 tools/taxonomy_poc.py src/menu/savemenu.c \
        asm/us/menu/nonmatchings/savemenu/func_801D224C.s

Whole corpus (writes build/taxonomy.jsonl):
    .venv/bin/python3 tools/taxonomy_poc.py --batch
"""

import argparse
import difflib
import json
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

AS_CMD = (
    "mipsel-linux-gnu-as -Iinclude -march=r3000 -mtune=r3000"
    " -no-pad-sections -O1 -G0"
)

CTX_CACHE = ROOT / "build" / "taxonomy_ctx"
RESULTS_PATH = ROOT / "build" / "taxonomy.jsonl"

M2C_MACROS = (ROOT / "tools" / "m2c" / "m2c_macros.h").read_text()

TIMEOUT = 120


def toolchain_for(src_file: Path) -> tuple[str, str, str]:
    line = src_file.read_text().splitlines()[0]
    cc1 = "cc1-psx-272"
    as_flags = "--expand-div --aspsx-version=2.34"
    if "PSYQ=3.3" in line:
        cc1 = "cc1-psx-26"
        as_flags = "--expand-div --aspsx-version=2.21"
    elif "PSYQ=4.0" in line:
        as_flags = "--expand-div --aspsx-version=2.56"
    if "CC1=2.6.3" in line:
        cc1 = "cc1-psx-26"
    elif "CC1=2.7.2" in line:
        cc1 = "cc1-psx-272"
    g_opt = "-G0"
    m = re.search(r"\bG=(\d+)", line)
    if m:
        g_opt = f"-G{m.group(1)}"
        as_flags += f" {g_opt}"
    return cc1, as_flags, g_opt


def run(cmd: str) -> bytes:
    result = subprocess.run(
        ["bash", "-o", "pipefail", "-c", cmd],
        cwd=ROOT,
        capture_output=True,
        timeout=TIMEOUT,
    )
    if result.returncode != 0:
        stderr = result.stderr.decode(errors="replace")
        errors = [
            line for line in stderr.splitlines() if "warning" not in line
        ]
        raise RuntimeError("\n".join(errors[-8:]) or f"exit: {cmd}")
    return result.stdout


def run_m2c(asm_file: Path, ctx: Path) -> str:
    return run(
        f".venv/bin/python3 tools/m2c/m2c.py --target mipsel-gcc-c"
        f" --context {ctx} --valid-syntax {asm_file}"
    ).decode()


def compile_candidate(
    c_code: str, ctx: Path, cc1: str, as_flags: str, g_opt: str,
    workdir: Path, func: str,
) -> Path:
    tu = workdir / "cand.c"
    # m2ctx re-appends the project #defines for decomp.me's benefit;
    # re-preprocessing them here expands names that collide with enum
    # members (e.g. BATTLE_SCENE), and m2c output uses literals anyway.
    # Also drop any prototype of the function under test: m2c may infer
    # a different signature than the project header declares, and the
    # conflict would mask an otherwise-comparable candidate.
    proto = re.compile(
        rf"^\w[^=]*\b{re.escape(func)}\s*\([^=]*\)\s*;\s*$"
    )
    ctx_decls = "".join(
        line
        for line in ctx.read_text().splitlines(keepends=True)
        if not line.startswith(("#define", "#undef"))
        and not proto.match(line)
    )
    # the project's types have no s64/u64 (PSX code never needs them),
    # but m2c_macros.h's M2C_UNK64 typedef does
    prelude = (
        "typedef long long s64;\ntypedef unsigned long long u64;\n"
        "#ifndef NULL\n#define NULL 0\n#endif\n"
    )
    tu.write_text(ctx_decls + prelude + M2C_MACROS + c_code)
    obj = workdir / "cand.o"
    run(
        f"mipsel-linux-gnu-cpp -Iinclude -Iinclude/psxsdk -lang-c"
        f" -undef -fno-builtin {tu}"
        f" | bin/{cc1} -quiet -mcpu=3000 -mgas -O2 {g_opt}"
        f" | python3 tools/maspsx/maspsx.py {as_flags}"
        f" | {AS_CMD} -o {obj}"
    )
    return obj


def assemble_target(asm_file: Path, workdir: Path) -> Path:
    s = workdir / "target.s"
    s.write_text('.include "macro.inc"\n.text\n' + asm_file.read_text())
    obj = workdir / "target.o"
    run(f"{AS_CMD} -I asm/us -o {obj} {s}")
    return obj


def disasm(obj: Path, func_hint: str | None = None) -> list[str]:
    out = run(f"mipsel-linux-gnu-objdump -d -m mips:3000 {obj}").decode()
    insns = []
    in_func = func_hint is None
    for line in out.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
        if m and func_hint:
            in_func = func_hint in m.group(1)
            continue
        m = re.match(r"^\s+[0-9a-f]+:\s+[0-9a-f]+\s+(\S+)\s*(.*)$", line)
        if m and in_func:
            mnemonic, ops = m.group(1), m.group(2)
            ops = re.sub(r"<[^>]*>", "", ops)
            ops = re.sub(r"0x[0-9a-f]+", "IMM", ops)
            ops = re.sub(r"-?\d+", "IMM", ops)
            insns.append(f"{mnemonic} {ops.strip()}")
    return insns


def blank_registers(insns: list[str]) -> list[str]:
    return [
        re.sub(
            r"\$?\b(zero|at|v[01]|a[0-3]|t[0-9]|s[0-7]|k[01]|gp|sp|fp|ra)\b",
            "REG",
            i,
        )
        for i in insns
    ]


FRAME_INSN = re.compile(
    r"^(sw|lw) \S+,IMM\(sp\)$|^addiu sp,sp,IMM$|^move |^nop"
)


def classify(cand: list[str], target: list[str]) -> tuple[str, float]:
    bc, bt = blank_registers(cand), blank_registers(target)
    similarity = difflib.SequenceMatcher(a=bc, b=bt).ratio()
    if cand == target:
        return "INSN_MATCH", 1.0
    if bc == bt:
        return "REGALLOC_ONLY", similarity

    # regalloc-dominated: every diff is either a register-only change
    # or an inserted/deleted frame op (save/restore, spill, move)
    sm = difflib.SequenceMatcher(a=cand, b=target)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        if tag == "replace" and (i2 - i1) == (j2 - j1):
            if bc[i1:i2] == bt[j1:j2]:
                continue
            return "STRUCTURAL", similarity
        gap = cand[i1:i2] + target[j1:j2]
        if not all(FRAME_INSN.match(insn) for insn in gap):
            return "STRUCTURAL", similarity
    return "REGALLOC_FRAME", similarity


def check_one(src_file: Path, asm_file: Path, ctx: Path) -> dict:
    func = asm_file.stem
    result = {
        "func": func,
        "asm": str(asm_file.relative_to(ROOT)),
        "src": str(src_file.relative_to(ROOT)),
        "decompiled": f" {func});" not in src_file.read_text(),
    }
    cc1, as_flags, g_opt = toolchain_for(src_file)
    try:
        with tempfile.TemporaryDirectory() as tmp:
            workdir = Path(tmp)
            target = disasm(assemble_target(asm_file, workdir), func)
            result["target_insns"] = len(target)
            try:
                c_code = run_m2c(asm_file, ctx)
            except (RuntimeError, subprocess.TimeoutExpired) as e:
                result.update(verdict="M2C_FAIL", error=str(e)[:300])
                return result
            try:
                obj = compile_candidate(
                    c_code, ctx, cc1, as_flags, g_opt, workdir, func
                )
            except (RuntimeError, subprocess.TimeoutExpired) as e:
                result.update(verdict="COMPILE_FAIL", error=str(e)[:300])
                return result
            cand = disasm(obj, func)
            verdict, similarity = classify(cand, target)
            result.update(
                verdict=verdict,
                similarity=round(similarity, 3),
                cand_insns=len(cand),
                cand_stream=cand,
                target_stream=target,
            )
    except (RuntimeError, subprocess.TimeoutExpired) as e:
        result.update(verdict="TARGET_ASM_FAIL", error=str(e)[:300])
    return result


def find_src_for(asm_dir_name: str) -> Path | None:
    hits = sorted(ROOT.glob(f"src/**/{asm_dir_name}.c"))
    return hits[0] if hits else None


def gen_context(src_file: Path) -> Path:
    cached = CTX_CACHE / (src_file.stem + ".ctx.c")
    if not cached.exists():
        run(f".venv/bin/python3 tools/m2ctx.py {src_file}")
        cached.write_text((ROOT / "ctx.c").read_text())
    return cached


def batch() -> int:
    CTX_CACHE.mkdir(parents=True, exist_ok=True)
    jobs = []
    for asm_file in sorted(ROOT.glob("asm/us/**/nonmatchings/**/*.s")):
        src_file = find_src_for(asm_file.parent.name)
        if src_file is None:
            print(f"skip (no src): {asm_file}", file=sys.stderr)
            continue
        jobs.append((src_file, asm_file))

    print(f"{len(jobs)} functions; generating contexts...", flush=True)
    contexts = {}
    for src_file in sorted({s for s, _ in jobs}):
        try:
            contexts[src_file] = gen_context(src_file)
        except (RuntimeError, subprocess.TimeoutExpired) as e:
            print(f"ctx failed for {src_file}: {e}", file=sys.stderr)

    jobs = [(s, a) for s, a in jobs if s in contexts]
    counts: dict[str, int] = {}
    done = 0
    with RESULTS_PATH.open("w") as out:
        with ProcessPoolExecutor(max_workers=8) as pool:
            futures = [
                pool.submit(check_one, s, a, contexts[s]) for s, a in jobs
            ]
            for fut in as_completed(futures):
                r = fut.result()
                out.write(json.dumps(r) + "\n")
                out.flush()
                counts[r["verdict"]] = counts.get(r["verdict"], 0) + 1
                done += 1
                if done % 50 == 0:
                    print(f"{done}/{len(jobs)} {counts}", flush=True)

    print(f"\nfinal: {done} functions")
    for verdict, n in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f"  {verdict:15} {n:5}  ({100 * n / done:.1f}%)")
    print(f"results: {RESULTS_PATH}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("src_file", type=Path, nargs="?")
    parser.add_argument("asm_file", type=Path, nargs="?")
    parser.add_argument("--batch", action="store_true")
    args = parser.parse_args()

    if args.batch:
        return batch()

    CTX_CACHE.mkdir(parents=True, exist_ok=True)
    ctx = gen_context(ROOT / args.src_file)
    result = check_one(ROOT / args.src_file, ROOT / args.asm_file, ctx)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
