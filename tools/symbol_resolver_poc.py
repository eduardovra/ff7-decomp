#!/usr/bin/env python3
"""Symbol-resolver PoC (idea #2): pre-m2c gate for one split .s file.

Cross-references every raw D_/func_ reference in the asm against the
project's symbol tables and headers, and reports the action for each:

  STALE_ASM   address is already named elsewhere -> re-split the asm
  MID_SYMBOL  address falls inside a named, sized symbol -> re-split
  DECLARED    named and declared in a header -> ensure m2c context sees it
  UNKNOWN     genuinely new -> suggested `mako.sh symbols add` command

Exit code is nonzero when anything needs action, so it can gate a
pipeline. Usage:

    .venv/bin/python3 tools/symbol_resolver_poc.py <asm_file.s>
"""

import argparse
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent

SYMBOL_LINE = re.compile(
    r"^(\w+)\s*=\s*0x([0-9A-Fa-f]{8});(?:\s*//\s*size:0x([0-9A-Fa-f]+))?"
)
RAW_REF = re.compile(r"\b((?:D|func|jtbl)_[0-9A-F]{8})\b")


def load_overlay_config() -> dict[str, list[str]]:
    cfg = yaml.safe_load((ROOT / "config" / "us.yaml").read_text())
    return {
        o["name"]: o.get("symbol_addrs_path", []) for o in cfg["overlays"]
    }


def load_symbols(paths: list[str]) -> dict[int, tuple[str, int, str]]:
    """addr -> (name, size, source file). Later files win on conflict."""
    table: dict[int, tuple[str, int, str]] = {}
    for path in paths:
        file = ROOT / path
        if not file.is_file():
            continue
        for line in file.read_text().splitlines():
            m = SYMBOL_LINE.match(line.strip())
            if not m:
                continue
            name, addr, size = m.group(1), int(m.group(2), 16), m.group(3)
            table[addr] = (name, int(size, 16) if size else 0, path)
    return table


def overlay_for(asm_file: Path, overlays: dict[str, list[str]]) -> str:
    for part in reversed(asm_file.parts):
        if part in overlays:
            return part
    raise SystemExit(f"cannot infer overlay from path: {asm_file}")


def find_declaration(name: str) -> str | None:
    decl = re.compile(rf"^[^/\n]*\b{re.escape(name)}\s*[\[;=(]")
    for header in list(ROOT.glob("include/**/*.h")) + list(
        ROOT.glob("src/**/*.h")
    ):
        for line in header.read_text(errors="replace").splitlines():
            if decl.match(line):
                return str(header.relative_to(ROOT))
    return None


def index_auto_name_declarations() -> dict[str, str]:
    """Auto-named (D_/func_) externs already typed somewhere in the
    tree -- the exact class m2c would otherwise re-invent untyped."""
    index: dict[str, str] = {}
    files = (
        list(ROOT.glob("include/**/*.h"))
        + list(ROOT.glob("src/**/*.h"))
        + list(ROOT.glob("src/**/*.c"))
    )
    for file in files:
        for line in file.read_text(errors="replace").splitlines():
            if "extern" not in line:
                continue
            for name in RAW_REF.findall(line):
                index.setdefault(name, str(file.relative_to(ROOT)))
    return index


def classify(
    ref: str,
    table: dict[int, tuple[str, int, str]],
    declared: dict[str, str],
) -> tuple[str, str]:
    addr = int(ref.split("_")[-1], 16)
    hit = table.get(addr)
    if hit and hit[0] != ref:
        name, _, source = hit
        header = find_declaration(name)
        where = f"declared in {header}" if header else "no declaration"
        return "STALE_ASM", f"named {name} in {source} ({where})"
    # inside a sized symbol that starts earlier?
    for base in sorted(a for a in table if a < addr):
        name, size, source = table[base]
        if size and base + size > addr and name != ref:
            return (
                "MID_SYMBOL",
                f"inside {name}+0x{addr - base:X} ({source})",
            )
    if ref in declared:
        return "DECLARED_IN_SRC", f"typed in {declared[ref]}"
    if hit:  # named exactly as the auto name -- known but placeholder
        return "KNOWN_PLACEHOLDER", f"listed in {hit[2]}"
    return "UNKNOWN", ""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("asm_file", type=Path)
    args = parser.parse_args()
    asm_file = (ROOT / args.asm_file).resolve()

    overlays = load_overlay_config()
    overlay = overlay_for(asm_file.relative_to(ROOT), overlays)
    # the overlay's own symbol files take priority (loaded last), but
    # every table is searched -- cross-overlay addresses are knowledge
    all_paths = [p for ps in overlays.values() for p in ps]
    own_paths = overlays[overlay]
    others = [p for p in dict.fromkeys(all_paths) if p not in own_paths]
    table = load_symbols(others + own_paths)

    refs = sorted(set(RAW_REF.findall(asm_file.read_text())))
    declared = index_auto_name_declarations()
    needs_action = 0
    symbols_file = next(
        (p for p in own_paths if "symbols." in p), own_paths[-1]
    )
    for ref in refs:
        verdict, detail = classify(ref, table, declared)
        print(f"{verdict:17} {ref}  {detail}")
        if verdict in ("STALE_ASM", "MID_SYMBOL"):
            needs_action += 1
        elif verdict == "UNKNOWN":
            needs_action += 1
            addr = ref.split("_")[-1]
            print(
                f"                  -> ./mako.sh symbols add"
                f" {symbols_file} {ref} 0x{addr}"
            )

    print(
        f"\n{len(refs)} refs: {needs_action} need action"
        f" (overlay {overlay})"
    )
    return 1 if needs_action else 0


if __name__ == "__main__":
    sys.exit(main())
