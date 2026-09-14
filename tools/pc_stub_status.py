#!/usr/bin/env python3
"""Classify the PC stubs of an upstream PR against this tree, by address.

Upstream's src/pc/stubs.c keeps whatever label the symbol had when it was
stubbed, so a name-based diff reports every function we have since named
while decompiling as still missing. Both sides are resolved to addresses
here instead.
"""

import argparse
import pathlib
import re
import subprocess
import sys
from typing import Dict, List, NamedTuple, Optional, Set

REPO = pathlib.Path(__file__).resolve().parent.parent
CONFIG = REPO / "config"
PSYZ_INCLUDE = REPO / "tools/psyz/psyz/include"

SYMBOL_RE = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9a-fA-F]+)\s*;")
ADDR_IN_NAME_RE = re.compile(r"^(?:func|D)_([0-9A-Fa-f]{8})$")
INCLUDE_ASM_RE = re.compile(r'INCLUDE_ASM\("([^"]+)",\s*(\w+)\)')
ASM_INSN_RE = re.compile(r"^\s*/\* [0-9A-Fa-f]+ ([0-9A-Fa-f]{8}) ")
HASM_LABEL_RE = re.compile(r"^([A-Za-z_]\w*):")
STUB_NAME_RE = re.compile(r"([A-Za-z_]\w*)\s*\(")
PROTOTYPE_RE = re.compile(
    r"^\s*(?:extern\s+)?[A-Za-z_][\w \t*]*?(\w+)\s*\([^;()]*\)\s*;"
)


class AsmFunc(NamedTuple):
    name: str
    addr: int
    path: pathlib.Path
    insns: int


class Stub(NamedTuple):
    name: str
    addr: Optional[int]
    state: str
    local: str
    detail: str


def fetch_diff(pr: int, repo: str) -> str:
    result = subprocess.run(
        ["gh", "pr", "diff", str(pr), "--repo", repo],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        sys.exit(f"gh pr diff failed: {result.stderr.strip()}")
    return result.stdout


def stubs_c_additions(diff: str) -> List[str]:
    """The lines src/pc/stubs.c gains in this diff."""
    lines: List[str] = []
    in_stubs = False
    for line in diff.splitlines():
        if line.startswith("diff --git "):
            in_stubs = line.endswith("b/src/pc/stubs.c")
            continue
        if in_stubs and line.startswith("+") and not line.startswith("+++"):
            lines.append(line[1:])
    return lines


def parse_stub_functions(diff: str) -> List[str]:
    """Names upstream stubs as functions: a one-line body ends in '}'."""
    names: List[str] = []
    for line in stubs_c_additions(diff):
        if not line.rstrip().endswith("}"):
            continue
        match = STUB_NAME_RE.search(line)
        if match is not None:
            names.append(match.group(1))
    return sorted(set(names))


def load_symbols() -> Dict[str, int]:
    symbols: Dict[str, int] = {}
    for path in sorted(CONFIG.glob("*.txt")):
        for line in path.read_text().splitlines():
            match = SYMBOL_RE.match(line)
            if match is not None:
                symbols.setdefault(match.group(1), int(match.group(2), 16))
    return symbols


def addr_of(name: str, symbols: Dict[str, int]) -> Optional[int]:
    """Prefer the address a func_/D_ label spells out over the tables."""
    match = ADDR_IN_NAME_RE.match(name)
    if match is not None:
        return int(match.group(1), 16)
    return symbols.get(name)


def names_by_addr(symbols: Dict[str, int]) -> Dict[int, List[str]]:
    """Reverse index; overlays share addresses, so values are lists."""
    index: Dict[int, List[str]] = {}
    for name, addr in symbols.items():
        index.setdefault(addr, []).append(name)
    for addr in index:
        index[addr].sort()
    return index


def read_asm_func(path: pathlib.Path) -> Optional[AsmFunc]:
    """Measure the function `path` is named after.

    A file can open with a .rodata jump table, whose entries look like
    instructions and carry unrelated addresses, so only the span between
    the function's own glabel and its .size counts.
    """
    if not path.is_file():
        return None
    addr: Optional[int] = None
    insns = 0
    in_body = False
    for line in path.read_text().splitlines():
        if line.startswith((f"glabel {path.stem}", f"{path.stem}:")):
            in_body = True
            continue
        if not in_body:
            continue
        if line.startswith(".size"):
            break
        match = ASM_INSN_RE.match(line)
        if match is None:
            continue
        insns += 1
        if addr is None:
            addr = int(match.group(1), 16)
    if addr is None:
        return None
    return AsmFunc(
        name=path.stem,
        addr=addr,
        path=path,
        insns=insns,
    )


def included_asm() -> Dict[int, List[AsmFunc]]:
    """Addresses still behind an INCLUDE_ASM the build actually compiles."""
    index: Dict[int, List[AsmFunc]] = {}
    for source in sorted(REPO.glob("src/**/*.c")):
        for directory, name in INCLUDE_ASM_RE.findall(source.read_text()):
            path = REPO / directory / f"{name}.s"
            func = read_asm_func(path)
            if func is None:
                continue
            index.setdefault(func.addr, []).append(func)
    return index


def split_asm() -> Dict[int, List[AsmFunc]]:
    """Every address splat has emitted asm for, included or not.

    An address with asm but no INCLUDE_ASM is compiled from C; one with no
    asm at all was never split, which is a different kind of not-done.
    """
    index: Dict[int, List[AsmFunc]] = {}
    for path in sorted(REPO.glob("asm/**/*.s")):
        func = read_asm_func(path)
        if func is not None:
            index.setdefault(func.addr, []).append(func)
    return index


def psyz_prototypes() -> Set[str]:
    """Functions PsyZ's public headers declare.

    A stub whose symbol PsyZ declares needs naming, not decompiling: once
    the label matches, the PC build links PsyZ's implementation.
    """
    names: Set[str] = set()
    if not PSYZ_INCLUDE.is_dir():
        return names
    for path in sorted(PSYZ_INCLUDE.glob("*.h")):
        for line in path.read_text(errors="replace").splitlines():
            if line.lstrip().startswith(("typedef", "#", "//", "*")):
                continue
            match = PROTOTYPE_RE.match(line)
            if match is not None:
                names.add(match.group(1))
    return names


def hand_written_asm() -> Set[str]:
    names: Set[str] = set()
    for source in sorted(REPO.glob("src/**/*.s")):
        for line in source.read_text().splitlines():
            match = HASM_LABEL_RE.match(line)
            if match is not None:
                names.add(match.group(1))
    return names


def classify(
    name: str,
    symbols: Dict[str, int],
    reverse: Dict[int, List[str]],
    asm: Dict[int, List[AsmFunc]],
    split: Dict[int, List[AsmFunc]],
    hasm: Set[str],
    psyz: Set[str],
) -> Stub:
    addr = addr_of(name, symbols)
    if addr is None:
        return Stub(name, None, "unsplit", "", "no address in any config file")

    local_names = [n for n in reverse.get(addr, []) if n != name]
    local = local_names[0] if local_names else name

    for candidate in [local] + local_names + [name]:
        if candidate in psyz:
            return Stub(
                name,
                addr,
                "psyz",
                candidate,
                f"PsyZ declares {candidate}; name it, do not decompile",
            )

    if addr in asm:
        entries = asm[addr]
        detail = ", ".join(
            f"{e.name} ({e.insns} insns) {e.path.parent.relative_to(REPO)}"
            for e in entries
        )
        return Stub(name, addr, "todo", entries[0].name, detail)

    for candidate in [name] + local_names:
        if candidate in hasm:
            return Stub(name, addr, "hasm", candidate, "hand-written asm")

    if addr in split:
        return Stub(name, addr, "done", local, f"decompiled as {local}")

    return Stub(name, addr, "unsplit", "", "no asm emitted at this address")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pr", type=int, default=167)
    parser.add_argument("--repo", default="Xeeynamo/ff7-decomp")
    parser.add_argument(
        "--diff",
        type=pathlib.Path,
        help="use a saved diff instead of calling gh",
    )
    parser.add_argument(
        "--state",
        choices=["todo", "psyz", "done", "hasm", "unsplit"],
        help="only report this state",
    )
    args = parser.parse_args()

    if args.diff is not None:
        diff = args.diff.read_text()
    else:
        diff = fetch_diff(pr=args.pr, repo=args.repo)

    symbols = load_symbols()
    reverse = names_by_addr(symbols)
    asm = included_asm()
    split = split_asm()
    hasm = hand_written_asm()
    psyz = psyz_prototypes()

    stubs = [
        classify(
            name=name,
            symbols=symbols,
            reverse=reverse,
            asm=asm,
            split=split,
            hasm=hasm,
            psyz=psyz,
        )
        for name in parse_stub_functions(diff)
    ]

    order = {"todo": 0, "psyz": 1, "hasm": 2, "unsplit": 3, "done": 4}
    stubs.sort(key=lambda s: (order[s.state], s.name))

    for stub in stubs:
        if args.state is not None and stub.state != args.state:
            continue
        addr = "????????" if stub.addr is None else f"{stub.addr:08X}"
        print(f"{stub.state:<7} {addr} {stub.name:<38} {stub.detail}")

    counts = {state: 0 for state in order}
    for stub in stubs:
        counts[stub.state] += 1
    summary = " ".join(f"{state}={counts[state]}" for state in order)
    print(f"\n{len(stubs)} function stubs: {summary}", file=sys.stderr)


if __name__ == "__main__":
    main()
