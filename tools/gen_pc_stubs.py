#!/usr/bin/env python3
"""Generate src/pc/stubs.c for the PSY-Z native prototype.

Every function still behind INCLUDE_ASM, and every global that only ever
existed as a linker-script address, is undefined when the decompiled C is
linked natively. This emits a placeholder for each one so the executable
links from day one and can be debugged while the real bodies land.

Run it against a built library, then rebuild:

    cmake --build build-pc --target ff7 psyz
    tools/gen_pc_stubs.py --psyz build-pc/psyz/libpsyz.a \\
        build-pc/libff7.a \\
        build-pc/CMakeFiles/ff7_pc.dir/src/pc/main.c.o
"""

import argparse
import pathlib
import re
import subprocess
import sys
from typing import Dict, Iterable, List, Set, Tuple

REPO = pathlib.Path(__file__).resolve().parent.parent
SYMBOL_RE = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9a-fA-F]+)\s*;")

# Fallback for when the system libraries cannot be read. Many PSY-Q names
# have identical C library semantics, and stubbing one shadows the real
# function for everything in the link, statically linked SDL included.
LIBC_FALLBACK = {
    "atoi",
    "memcpy",
    "memset",
    "mmap",
    "printf",
    "rand",
    "sprintf",
    "srand",
    "strcmp",
}

# PS1 file I/O, which PSY-Z does not provide. These share a name with the
# POSIX calls but not their semantics, and a stub here would override the
# real ones for statically linked SDL too. Left undefined on purpose: they
# have to be routed to romio once disc loading lands.
PS1_FILE_IO = {"close", "open", "read", "write"}

# Real definitions live in src/pc/globals.c, where the header knows the
# true size. A guessed byte array would be too small and corrupt the heap.
HAND_DEFINED = {"Savemap"}

DEFAULT_DATA_SIZE = 256
MAX_DATA_SIZE = 0x20000


def run_nm(args: List[str]) -> List[str]:
    result = subprocess.run(
        ["nm"] + args,
        capture_output=True,
        text=True,
        check=True,
    )
    return result.stdout.splitlines()


def undefined_symbols(objects: Iterable[pathlib.Path]) -> Set[str]:
    undefined: Set[str] = set()
    for obj in objects:
        lines = run_nm(["-u", "--no-demangle", str(obj)])
        undefined |= {
            parts[1] for parts in map(str.split, lines) if len(parts) == 2
        }
    return undefined


def defined_symbols(archives: Iterable[pathlib.Path]) -> Set[str]:
    defined: Set[str] = set()
    for archive in archives:
        lines = run_nm(["--defined-only", str(archive)])
        defined |= {
            parts[2] for parts in map(str.split, lines) if len(parts) == 3
        }
    return defined


def libc_symbols() -> Set[str]:
    """Everything the system C library already defines."""
    symbols: Set[str] = set()
    for name in ("libc.so.6", "libm.so.6"):
        try:
            found = subprocess.run(
                ["cc", "-print-file-name=" + name],
                capture_output=True,
                text=True,
                check=True,
            ).stdout.strip()
            lines = run_nm(["-D", "--defined-only", found])
        except (OSError, subprocess.CalledProcessError):
            continue
        for parts in map(str.split, lines):
            if len(parts) == 3:
                symbols.add(parts[2].split("@")[0])
    if not symbols:
        return LIBC_FALLBACK
    return symbols | LIBC_FALLBACK


def asm_function_names() -> Set[str]:
    return {path.stem for path in (REPO / "asm").rglob("*.s")}


CALL_RE = re.compile(r"\b(\w+)\s*\(")


def called_names() -> Set[str]:
    """Names used as a call or declared as a prototype anywhere in the C."""
    names: Set[str] = set()
    for root in ("src", "include"):
        for path in (REPO / root).rglob("*.[ch]"):
            names |= set(CALL_RE.findall(path.read_text(errors="ignore")))
    return names


def config_addresses() -> List[Dict[str, int]]:
    """Symbol addresses, kept one dict per file so overlays stay separate."""
    per_file = []
    for path in sorted((REPO / "config").glob("*.txt")):
        addresses = {}
        for line in path.read_text().splitlines():
            match = SYMBOL_RE.match(line)
            if match:
                addresses[match.group(1)] = int(match.group(2), 16)
        if addresses:
            per_file.append(addresses)
    return per_file


def data_sizes() -> Dict[str, int]:
    """Size each global as the gap to the next symbol in the same file."""
    sizes: Dict[str, int] = {}
    for addresses in config_addresses():
        ordered = sorted(addresses.items(), key=lambda item: item[1])
        for index, (name, address) in enumerate(ordered):
            if index + 1 < len(ordered):
                gap = ordered[index + 1][1] - address
            else:
                gap = DEFAULT_DATA_SIZE
            if gap < 4:
                gap = 4
            if gap > MAX_DATA_SIZE:
                gap = MAX_DATA_SIZE
            sizes[name] = max(sizes.get(name, 0), gap)
    return sizes


def classify(
    missing: Set[str],
) -> Tuple[List[str], List[Tuple[str, int]]]:
    """Split the missing symbols into functions and sized globals."""
    callable_names = asm_function_names() | called_names()
    sizes = data_sizes()
    functions = []
    globals_ = []
    for name in sorted(missing):
        if name in callable_names:
            functions.append(name)
        else:
            globals_.append((name, sizes.get(name, DEFAULT_DATA_SIZE)))
    return functions, globals_


def render(
    functions: List[str],
    globals_: List[Tuple[str, int]],
) -> str:
    lines = [
        "// Generated by tools/gen_pc_stubs.py -- do not edit by hand.",
        "//",
        "// Placeholders that let the native PSY-Z build link before every",
        "// function is decompiled. A stubbed function returns 0 and a",
        "// stubbed global reads as zero, so anything that reaches one",
        "// behaves wrongly rather than failing loudly.",
        "",
        "#define STUB_FUNC(name) int name(void) { return 0; }",
        "#define STUB_DATA(name, size)"
        " __attribute__((aligned(8))) unsigned char name[size]",
        "",
        f"// {len(globals_)} globals",
    ]
    for name, size in globals_:
        lines.append(f"STUB_DATA({name}, {size});")
    lines += ["", f"// {len(functions)} functions"]
    for name in functions:
        lines.append(f"STUB_FUNC({name})")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "objects",
        type=pathlib.Path,
        nargs="+",
        help="built libraries and objects to read undefined symbols from",
    )
    parser.add_argument(
        "--psyz",
        type=pathlib.Path,
        action="append",
        default=[],
        help="a libpsyz.a whose symbols must not be stubbed",
    )
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        default=REPO / "src" / "pc" / "stubs.c",
    )
    args = parser.parse_args()

    provided = (
        defined_symbols(args.psyz)
        | libc_symbols()
        | PS1_FILE_IO
        | HAND_DEFINED
    )
    # An archive member can reference what another member defines, so those
    # show up as undefined too. Stubbing one would shadow the real body.
    provided |= defined_symbols(args.objects)
    missing = {
        name
        for name in undefined_symbols(args.objects) - provided
        if not name.startswith("_")
    }
    functions, globals_ = classify(missing)
    args.output.write_text(render(functions, globals_))
    print(
        f"{args.output}: {len(functions)} functions, "
        f"{len(globals_)} globals"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
