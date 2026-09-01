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
import subprocess
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent

SYMBOL_LINE = re.compile(
    r"^(\w+)\s*=\s*0x([0-9A-Fa-f]{8});(?:\s*//\s*size:0x([0-9A-Fa-f]+))?"
)
RAW_REF = re.compile(r"\b((?:D|func|jtbl)_[0-9A-F]{8})\b")


def load_overlay_config() -> dict[str, dict]:
    cfg = yaml.safe_load((ROOT / "config" / "us.yaml").read_text())
    return {
        o["name"]: {
            "symbol_paths": o.get("symbol_addrs_path", []),
            "vram_start": o["vram_start"],
        }
        for o in cfg["overlays"]
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


def overlay_for(asm_file: Path, overlays: dict[str, dict]) -> str:
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


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True)


def write_externs_header(
    overlay: str, decls: list[tuple[str, str]]
) -> Path | None:
    """Copy scattered typed declarations into one header the m2c
    context generation can be pointed at."""
    if not decls:
        return None
    out = ROOT / "build" / "resolver_externs" / f"{overlay}.h"
    out.parent.mkdir(parents=True, exist_ok=True)
    lines = []
    for ref, source in decls:
        for line in (ROOT / source).read_text(errors="replace").splitlines():
            if "extern" in line and re.search(rf"\b{ref}\b", line):
                lines.append(line.strip())
                break
    out.write_text("\n".join(dict.fromkeys(lines)) + "\n")
    return out


def delete_orphans(asm_dir: Path, src_file: Path) -> list[str]:
    """Remove function .s files no longer referenced by an INCLUDE_ASM
    -- leftovers from renames and finished decompilation. Data (D_*)
    and jump-table files are kept."""
    included = set(
        re.findall(r"INCLUDE_ASM\([^,]+,\s*(\w+)\)", src_file.read_text())
    )
    removed = []
    for s in sorted(asm_dir.glob("*.s")):
        if s.stem.startswith(("D_", "jtbl_")):
            continue
        if s.stem not in included:
            s.unlink()
            removed.append(s.name)
    return removed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("asm_file", type=Path)
    parser.add_argument(
        "--fix",
        action="store_true",
        help="apply actions: delete orphaned .s files, write the"
        " externs header, re-split the overlay if asm is stale."
        " UNKNOWN refs stay report-only (naming needs a human/LLM)",
    )
    args = parser.parse_args()
    asm_file = (ROOT / args.asm_file).resolve()

    overlays = load_overlay_config()
    overlay = overlay_for(asm_file.relative_to(ROOT), overlays)
    if args.fix:
        # clean orphans before classifying: a stale input file's refs
        # must not be registered as symbols
        src_hits = sorted(ROOT.glob(f"src/**/{asm_file.parent.name}.c"))
        if src_hits:
            for name in delete_orphans(asm_file.parent, src_hits[0]):
                print(f"deleted orphan: {name}")
        if not asm_file.exists():
            print(f"{asm_file.name} was an orphan -- nothing to resolve")
            return 0

    # search only address spaces this overlay can legitimately see:
    # its own files, main (always resident), and files shared by
    # several overlays. Other overlays occupy the same RAM, so their
    # private symbols would be false positives here.
    own_paths = overlays[overlay]["symbol_paths"]
    main_paths = overlays["main"]["symbol_paths"]
    all_paths = [
        p for o in overlays.values() for p in o["symbol_paths"]
    ]
    shared = [p for p in dict.fromkeys(all_paths) if all_paths.count(p) > 1]
    table = load_symbols(
        list(dict.fromkeys(shared + main_paths + own_paths))
    )

    refs = sorted(set(RAW_REF.findall(asm_file.read_text())))
    declared = index_auto_name_declarations()
    needs_action = 0
    needs_resplit = False
    src_decls: list[tuple[str, str]] = []
    for ref in refs:
        verdict, detail = classify(ref, table, declared)
        print(f"{verdict:17} {ref}  {detail}")
        if verdict in ("STALE_ASM", "MID_SYMBOL"):
            needs_action += 1
            needs_resplit = True
        elif verdict == "DECLARED_IN_SRC":
            src_decls.append((ref, detail.removeprefix("typed in ")))
        elif verdict == "UNKNOWN":
            # report only: splat's auto-name is already a stable
            # placeholder, so a symbols-add entry earns its place only
            # with a real name or a size -- both need a human/LLM
            needs_action += 1

    print(
        f"\n{len(refs)} refs: {needs_action} need action"
        f" (overlay {overlay})"
    )
    if not args.fix:
        return 1 if needs_action else 0

    header = write_externs_header(overlay, src_decls)
    if header:
        rel = header.relative_to(ROOT)
        print(f"externs header for m2c context: {rel}")
    if needs_resplit:
        run(["ninja", f"build/us/{overlay}.ld"])
        print(f"re-split {overlay}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
